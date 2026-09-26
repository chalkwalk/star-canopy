#include "galaxy.h"

#include "noise.h"
#include "random.h"

#include <cmath>
#include <cstring>

namespace starcanopy {

namespace {

constexpr float kPi = 3.14159265358979323846f;

float smoothstep(float e0, float e1, float x) {
  float t = (x - e0) / (e1 - e0);
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
  return t * t * (3.0f - 2.0f * t);
}

// The shader's nsky_noise(), channel c: the same periodic noise the volume was
// baked from.
float noise(const float p[3], int c) {
  return periodicNoise(p[0], p[1], p[2], kNoisePeriod, 7919u * static_cast<uint32_t>(c + 1));
}

}  // namespace

Galaxy generateGalaxy(uint32_t seed, const GalaxyParams& p) {
  Galaxy g{};
  Random rng(seed * 2654435789u + 2027u);
  float a[3], b[3];

  // The galaxy's orientation in the sky owes nothing to anything else's.
  rng.unitVector(a);
  rng.perpendicular(a, b);
  std::memcpy(&g.rot[0], b, sizeof(b));
  g.rot[3] = a[1] * b[2] - a[2] * b[1];
  g.rot[4] = a[2] * b[0] - a[0] * b[2];
  g.rot[5] = a[0] * b[1] - a[1] * b[0];
  std::memcpy(&g.rot[6], a, sizeof(a));

  g.scaleLength = rng.range(2.2f, 3.2f);
  g.scaleHeight = 0.3f;
  g.flareStart = 3.0f * g.scaleLength;
  g.flareLength = 3.5f;
  g.edge = 5.5f * g.scaleLength;

  float azimuth = rng.range(0.0f, 2.0f * kPi);
  g.observer[0] = p.observerRadius * g.scaleLength * cosf(azimuth);
  g.observer[1] = p.observerRadius * g.scaleLength * sinf(azimuth);
  g.observer[2] = p.observerHeight;

  g.armPhase = rng.range(0.0f, 2.0f * kPi);
  g.barAngle = rng.range(0.0f, kPi);
  switch (p.style) {
    case kGrandDesign:
      g.arms = 2.0f;
      g.pitchTan = tanf(rng.range(12.0f, 18.0f) * kPi / 180.0f);
      g.armStrength = 1.6f;
      g.armSharpness = 5.0f;
      g.bulgeStrength = 1.2f;
      break;
    case kFlocculent:
      g.arms = 3.0f;
      g.pitchTan = tanf(rng.range(18.0f, 28.0f) * kPi / 180.0f);
      g.armStrength = 1.2f;
      g.armSharpness = 2.0f;
      g.flocculence = 0.9f;
      g.bulgeStrength = 0.6f;
      break;
    default:
      g.arms = rng.uniform() < 0.5f ? 2.0f : 4.0f;
      g.pitchTan = tanf(rng.range(10.0f, 14.0f) * kPi / 180.0f);
      g.armStrength = 1.0f;
      g.armSharpness = 3.0f;
      g.flocculence = 0.15f;
      g.barLength = rng.range(3.0f, 4.5f);
      g.barStrength = 1.0f;
      g.bulgeStrength = 1.0f;
      break;
  }

  g.dust = 8.0f * p.dust;
  g.dustHeight = 0.1f;

  // The warp begins well out and grows as the square of the distance past
  // that, up on one side and down on the other; the waves ride on it.
  g.warp = 0.02f * p.warp;
  g.warpStart = 3.5f * g.scaleLength;
  g.warpPhase = rng.range(0.0f, 2.0f * kPi);
  g.waves = 0.15f * p.waves;
  g.waveLength = rng.range(2.5f, 4.0f);
  g.wavePhase = rng.range(0.0f, 2.0f * kPi);

  g.externalCount = p.externalGalaxies > kMaxExternalGalaxies ? kMaxExternalGalaxies
                                                              : p.externalGalaxies;
  for (int i = 0; i < g.externalCount; i++) {
    ExternalGalaxy& e = g.external[i];
    rng.unitVector(e.dir);
    rng.perpendicular(e.dir, e.major);
    // Mostly small, the odd one a degree or two, as Andromeda is.
    e.radius = expf(rng.range(logf(0.15f), logf(1.8f))) * kPi / 180.0f;
    e.axisRatio = rng.range(0.15f, 1.0f);
    e.brightness = rng.range(0.3f, 1.0f);
  }
  return g;
}

void galaxyToFrame(const Galaxy& g, const float sky[3], float out[3]) {
  for (int i = 0; i < 3; i++) {
    out[i] = g.rot[i * 3 + 0] * sky[0] + g.rot[i * 3 + 1] * sky[1] + g.rot[i * 3 + 2] * sky[2];
  }
}

GalaxySample galaxyDensity(const Galaxy& g, const float p[3]) {
  GalaxySample s{};
  float rDisc = sqrtf(p[0] * p[0] + p[1] * p[1]);
  float r = sqrtf(dot3(p, p));
  float phi = atan2f(p[1], p[0]);
  float q[3];

  // The midplane: warp, then the bending waves on top of it.
  float zmid = g.warp * powf(fmaxf(rDisc - g.warpStart, 0.0f), 2.0f) * sinf(phi - g.warpPhase);
  zmid += g.waves * sinf(2.0f * kPi * rDisc / g.waveLength + g.wavePhase) *
          smoothstep(g.warpStart - 3.0f, g.warpStart + 1.0f, rDisc) *
          cosf(phi - 0.5f * g.wavePhase);
  float h = fminf(g.scaleHeight * expf(fmaxf(rDisc - g.flareStart, 0.0f) / g.flareLength), 2.5f);
  float dz = p[2] - zmid;
  float radial = expf(-rDisc / g.scaleLength) * (1.0f - smoothstep(0.85f * g.edge, g.edge, rDisc));

  float psi = g.arms * (phi - logf(fmaxf(rDisc, 0.5f)) / g.pitchTan) + g.armPhase;
  float arm = powf(0.5f + 0.5f * cosf(psi), g.armSharpness);
  float clump;
  if (g.flocculence > 0.0f) {
    q[0] = p[0] * 0.7f;
    q[1] = p[1] * 0.7f;
    q[2] = p[2] * 0.7f;
    clump = fminf(fmaxf(0.5f + 1.5f * noise(q, 0), 0.0f), 1.6f);
    arm *= 1.0f + g.flocculence * (clump - 1.0f);
  }
  if (g.barLength > 0.0f) {
    arm *= smoothstep(0.8f * g.barLength, 1.3f * g.barLength, rDisc);
  }

  // Star clouds: the disc's light is lumpy on the scale of a kiloparsec, which
  // is what mottles a band seen from inside; the young stars more so.
  q[0] = p[0] * 1.1f + 3.0f;
  q[1] = p[1] * 1.1f + 3.0f;
  q[2] = p[2] * 1.1f + 3.0f;
  clump = fminf(fmaxf(0.6f + 0.9f * noise(q, 2), 0.2f), 1.6f);
  s.old = radial * expf(-(dz * dz) / (h * h)) / h * (1.0f + 0.6f * g.armStrength * arm) * clump;
  float hy = 0.3f * h;
  s.young = radial * expf(-(dz * dz) / (hy * hy)) / hy * g.armStrength * arm * 0.25f * clump * clump;

  if (g.barStrength > 0.0f) {
    float qx = p[0] * cosf(g.barAngle) + p[1] * sinf(g.barAngle);
    float qy = -p[0] * sinf(g.barAngle) + p[1] * cosf(g.barAngle);
    float rb2 = (qx * qx) / (g.barLength * g.barLength) +
                (qy * qy) / (0.1225f * g.barLength * g.barLength) +
                (p[2] * p[2]) / (0.0625f * g.barLength * g.barLength);
    s.old += g.barStrength * 0.5f * expf(-2.5f * rb2);
  }
  s.old += g.bulgeStrength * 0.6f * expf(-r / 1.0f);
  s.old += 0.0008f / powf(1.0f + r * r / 4.0f, 1.5f);

  // Two octaves, the finer one strong, so the dust breaks into lanes and
  // filaments rather than lying in soft sheets.
  q[0] = p[0] * 1.3f;
  q[1] = p[1] * 1.3f;
  q[2] = p[2] * 1.3f;
  clump = 0.3f + 1.0f * noise(q, 1);
  q[0] = p[0] * 4.1f + 11.0f;
  q[1] = p[1] * 4.1f + 11.0f;
  q[2] = p[2] * 4.1f + 11.0f;
  clump = fminf(fmaxf(clump + 0.9f * noise(q, 3), 0.0f), 2.5f);
  float dh = g.dustHeight * h / g.scaleHeight;
  s.dust = g.dust * expf(-rDisc / (1.5f * g.scaleLength)) *
           (1.0f - smoothstep(0.8f * g.edge, g.edge, rDisc)) * expf(-(dz * dz) / (dh * dh)) *
           (0.3f + 1.2f * arm) * clump;
  return s;
}

}  // namespace starcanopy
