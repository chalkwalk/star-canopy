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

// The dust's fractal, a lognormal of GAL_DUST_OCTAVES octaves; see
// gal_dust_field() in galaxy.glsl, which this must match.
constexpr int kDustOctaves = 7;
constexpr float kDustGain = 0.75f, kDustNorm = 2.2448f;
constexpr int kDustRidged = 2;

float dustField(const float p[3], float footprint, float sigma) {
  float f = 2.6f, a = 1.0f, sum = 0.0f, kept = 0.0f, scale = 1.0f / sqrtf(0.073f * kDustNorm);
  for (int k = 0; k < kDustOctaves; k++) {
    float w = fminf(fmaxf(1.0f / (f * 2.0f * footprint) - 1.0f, 0.0f), 1.0f);
    if (w > 0.0f) {
      float q[3];
      for (int i = 0; i < 3; i++) {
        q[i] = p[i] * f + static_cast<float>(k) * 7.31f;
      }
      float v = noise(q, k % 4);
      if (k >= kDustRidged) {
        v = (0.219f - fabsf(v)) * 1.72f;
      }
      sum += a * w * v;
      kept += a * a * w * w;
    }
    a *= kDustGain;
    f *= 2.07f;
  }
  return expf(sigma * sum * scale - 0.5f * sigma * sigma * kept / kDustNorm);
}

}  // namespace

int galaxyStyleFor(uint32_t seed) {
  Random rng(seed * 3266489917u + 1231u);
  float u = rng.uniform();
  return u < 0.35f ? kBarredSpiral : u < 0.5f ? kGrandDesign : u < 0.75f ? kFlocculent : kLenticular;
}

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
  g.bulgeRadius = 1.0f;
  g.bulgeFlattening = 2.0f;
  float lenticularDust = 1.0f;
  g.barAngle = rng.range(0.0f, kPi);
  int style = p.style >= 0 && p.style < kGalaxyStyles ? p.style : galaxyStyleFor(seed);
  switch (style) {
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
    case kLenticular: {
      // A disc that has stopped making stars: its arms faded with their young
      // stars to a trace, and what it has instead is concentric -- a large
      // bulge, a lens with a sharp edge in nearly all, a bar in many, rings at
      // the bar's size and twice it, and dust, when there is any, in thin
      // rings about the centre (docs/studies/atlas.md). From a stream of its
      // own, so the draws after are a spiral's.
      Random s0(seed * 2891336453u + 1597u);
      g.arms = 2.0f;
      g.pitchTan = tanf(s0.range(8.0f, 12.0f) * kPi / 180.0f);
      g.armStrength = s0.range(0.01f, 0.03f);
      g.armSharpness = 2.0f;
      g.bulgeStrength = s0.range(1.2f, 1.8f);
      g.bulgeRadius = s0.range(1.6f, 2.4f);
      g.bulgeFlattening = 1.4f;
      if (s0.uniform() < 0.5f) {
        g.barLength = s0.range(3.0f, 4.5f);
        g.barStrength = s0.range(2.0f, 3.0f);
        g.lensRadius = 1.3f * g.barLength;
      } else {
        g.lensRadius = s0.range(0.9f, 1.5f) * g.scaleLength;
      }
      g.lensStrength = s0.range(0.2f, 0.4f);
      g.innerRing = s0.uniform() < 0.5f ? s0.range(0.5f, 1.5f) : 0.0f;
      g.outerRing = s0.uniform() < 0.3f ? s0.range(0.5f, 1.5f) : 0.0f;
      // Dust from none to a few rings: the S0 classes by dust.
      if (s0.uniform() < 0.6f) {
        lenticularDust = s0.range(0.3f, 1.0f);
        g.dustRing = s0.range(0.3f, 0.9f) * g.lensRadius;
      } else {
        lenticularDust = 0.05f;
      }
      break;
    }
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

  g.dust = 8.0f * p.dust * lenticularDust;
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
  // From a stream of its own, so the number of external galaxies, which
  // decides how many draws come before, cannot change it.
  Random dustRng(seed * 2246822519u + 3079u);
  g.dustSigma = dustRng.range(1.2f, 2.6f);
  return g;
}

// The galaxy's dust between the observer and a point offset from it, kpc.
float dustDepth(const Galaxy& g, const float offset[3]) {
  const int steps = 16;
  float p[3];
  float len = sqrtf(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
  float tau = 0.0f;
  for (int i = 0; i < steps; i++) {
    float t = (static_cast<float>(i) + 0.5f) / static_cast<float>(steps);
    for (int k = 0; k < 3; k++) {
      p[k] = g.observer[k] + offset[k] * t;
    }
    // The dust's detail no finer than a step: finer, and a step lands in a
    // cloud or misses it by chance.
    tau += galaxyDensity(g, p, len / static_cast<float>(steps)).dust;
  }
  return tau * len / static_cast<float>(steps);
}

void galaxyToFrame(const Galaxy& g, const float sky[3], float out[3]) {
  for (int i = 0; i < 3; i++) {
    out[i] = g.rot[i * 3 + 0] * sky[0] + g.rot[i * 3 + 1] * sky[1] + g.rot[i * 3 + 2] * sky[2];
  }
}

GalaxySample galaxyDensity(const Galaxy& g, const float p[3], float footprint) {
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

  // An arm meanders, widens and narrows, comes in segments and breaks up
  // toward its end; see gal_density().
  auto at = [&](float scale, float offset, int channel) {
    float r[3] = {p[0] * scale + offset, p[1] * scale + offset, p[2] * scale + offset};
    return noise(r, channel);
  };
  float psi = g.arms * (phi - logf(fmaxf(rDisc, 0.5f)) / g.pitchTan) + g.armPhase +
              0.3f * at(0.3f, 23.0f, 2);
  float width = g.armSharpness * expf(0.5f * at(0.4f, 17.0f, 0));
  float arm = powf(0.5f + 0.5f * cosf(psi), width);
  // The dust lane on the arm's inner edge; see gal_density().
  float lane = powf(0.5f + 0.5f * cosf(psi - 0.5f), 1.5f * width);
  float segment = fminf(fmaxf(1.0f + 1.2f * at(0.35f, 11.0f, 1), 0.5f), 1.5f);
  float outer = smoothstep(2.0f * g.scaleLength, 4.0f * g.scaleLength, rDisc);
  segment *= 1.0f + outer * (fminf(fmaxf(0.3f + 2.5f * at(0.9f, 29.0f, 3), 0.0f), 1.8f) - 1.0f);
  arm *= segment;
  lane *= 0.5f + 0.5f * segment;
  float clump;
  if (g.flocculence > 0.0f) {
    q[0] = p[0] * 0.7f;
    q[1] = p[1] * 0.7f;
    q[2] = p[2] * 0.7f;
    clump = fminf(fmaxf(0.5f + 1.5f * noise(q, 0), 0.0f), 1.6f);
    arm *= 1.0f + g.flocculence * (clump - 1.0f);
    lane *= 1.0f + g.flocculence * (clump - 1.0f);
  }
  if (g.barLength > 0.0f) {
    float inner = smoothstep(0.8f * g.barLength, 1.3f * g.barLength, rDisc);
    arm *= inner;
    lane *= inner;
  }

  // Star clouds: the disc's light is lumpy on the scale of a kiloparsec, which
  // is what mottles a band seen from inside; the young stars more so.
  q[0] = p[0] * 1.1f + 3.0f;
  q[1] = p[1] * 1.1f + 3.0f;
  q[2] = p[2] * 1.1f + 3.0f;
  // A smooth old disc; see gal_density().
  clump = fminf(fmaxf(1.0f + 0.8f * noise(q, 2), 0.7f), 1.3f);
  float stirred = fminf(4.0f * g.armStrength, 1.0f);
  clump = 1.0f * (1.0f - stirred) + clump * stirred;
  // The arms, strong enough to be seen; see gal_density().
  // Dead space between the arms in the outer disc; see gal_density().
  float between = outer * fminf(g.armStrength, 1.0f);
  s.old = radial * expf(-(dz * dz) / (h * h)) / h * clump *
          ((1.0f + 1.2f * g.armStrength * arm) * (1.0f - between) +
           (0.3f + 2.4f * g.armStrength * arm) * between);
  float hy = 0.3f * h;
  // The young stars in knots along the arms; see gal_density().
  for (int i = 0; i < 3; i++) {
    q[i] = p[i] * 4.0f + 7.0f;
  }
  float knot = fminf(fmaxf(0.4f + 2.2f * noise(q, 3), 0.0f), 2.5f);
  // Sharper, and reaching past the old disc; see gal_density().
  s.young = expf(-rDisc / (1.6f * g.scaleLength)) *
            (1.0f - smoothstep(0.85f * g.edge, g.edge, rDisc)) * expf(-(dz * dz) / (hy * hy)) / hy *
            g.armStrength * powf(arm, 1.5f) * 1.2f * knot * knot;

  if (g.barStrength > 0.0f) {
    float qx = p[0] * cosf(g.barAngle) + p[1] * sinf(g.barAngle);
    float qy = -p[0] * sinf(g.barAngle) + p[1] * cosf(g.barAngle);
    float rb2 = (qx * qx) / (g.barLength * g.barLength) +
                (qy * qy) / (0.1225f * g.barLength * g.barLength) +
                (p[2] * p[2]) / (0.0625f * g.barLength * g.barLength);
    s.old += g.barStrength * 0.5f * expf(-2.5f * rb2);
  }
  // The bulge, flattened; see gal_density().
  s.old += g.bulgeStrength * 0.45f *
           expf(-sqrtf(p[0] * p[0] + p[1] * p[1] +
                       g.bulgeFlattening * g.bulgeFlattening * p[2] * p[2]) /
                g.bulgeRadius);
  // A lenticular's lens and rings; see gal_density().
  if (g.lensRadius > 0.0f) {
    float ri = g.lensRadius / 1.3f, ro = 2.0f * ri;
    float vert = expf(-(dz * dz) / (h * h)) / h;
    float di = (rDisc - ri) / (0.08f * ri), dout = (rDisc - ro) / (0.1f * ro);
    s.old += vert * (g.lensStrength * expf(-rDisc / (4.0f * g.scaleLength)) *
                         (1.0f - smoothstep(0.92f * g.lensRadius, g.lensRadius, rDisc)) +
                     radial * (g.innerRing * expf(-di * di) + g.outerRing * expf(-dout * dout)));
  }
  s.old += 0.0008f / powf(1.0f + r * r / 4.0f, 1.5f);

  // The dust layer, thin but not flat: its thickness and its middle vary by
  // low noises, so near clouds stand out of the plane. See gal_density().
  for (int i = 0; i < 3; i++) {
    q[i] = p[i] * 0.9f + 5.0f;
  }
  float dh = g.dustHeight * h / g.scaleHeight * expf(1.2f * noise(q, 0));
  for (int i = 0; i < 3; i++) {
    q[i] = p[i] * 0.5f + 9.0f;
  }
  dz -= 0.3f * noise(q, 1);
  s.dust = g.dust * 0.4f * expf(-rDisc / (1.5f * g.scaleLength)) *
           (1.0f - smoothstep(0.8f * g.edge, g.edge, rDisc)) * expf(-(dz * dz) / (dh * dh)) *
           (0.25f + 1.4f * g.armStrength * lane) * dustField(p, footprint, g.dustSigma);
  // A lenticular's dust in rings; see gal_density().
  if (g.dustRing > 0.0f) {
    float r1 = g.dustRing, r2 = 1.7f * g.dustRing;
    float d1 = (rDisc - r1) / (0.12f * r1), d2 = (rDisc - r2) / (0.1f * r2);
    s.dust *= 0.05f + 4.0f * (expf(-d1 * d1) + 0.6f * expf(-d2 * d2));
  }
  return s;
}

}  // namespace starcanopy
