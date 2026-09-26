#include "stars.h"

#include "random.h"

#include <cmath>
#include <cstring>

namespace starcanopy {

namespace {

// The brightest a star may be, in multiples of a typical one; a star a few
// light years off would otherwise come out as bright as a sun.
constexpr float kMaxFlux = 20000.0f;
// Nothing nearer than this, kpc: the observer's own neighbourhood is not a
// star field.
constexpr float kMinDistance = 0.004f;

// Mostly cool stars, since those are most of them; hot ones rare. The bright
// end of a real naked-eye sky is biased hot, because hot stars are luminous, so
// the chance of hot rises with brightness.
float starTemperature(Random& rng, float brightness) {
  float hotChance = 0.04f + 0.3f * fminf(1.0f, brightness / 200.0f);
  float u = rng.uniform();
  if (u < hotChance) {
    return rng.range(10000.0f, 30000.0f);
  }
  if (u < hotChance + 0.25f) {
    return rng.range(6000.0f, 10000.0f);
  }
  return rng.range(3000.0f, 6000.0f);
}

void setFlux(Star& s, float kelvin, float flux) {
  float rgb[3];
  blackbody(kelvin, rgb);
  for (int i = 0; i < 3; i++) {
    s.flux[i] = rgb[i] * flux;
  }
}

void addClusterStars(Random& rng, std::vector<Star>& stars, const Scene& s, const StarParams& p) {
  for (int i = 0; i < s.bubbleCount; i++) {
    const Bubble& b = s.bubble[i];
    for (const Cluster& cl : b.cluster) {
      if (cl.luminosity <= 0.0f) {
        continue;
      }
      float center[3];
      bubbleToSky(b, cl.pos, center);
      float dist = sqrtf(center[0] * center[0] + center[1] * center[1] + center[2] * center[2]);

      // The lighting stars themselves.
      for (const ClusterStar& cs : cl.star) {
        Star st{};
        std::memcpy(st.dir, cs.dir, sizeof(st.dir));
        st.distance = dist;
        setFlux(st, rng.range(25000.0f, 45000.0f), cs.intensity * 4000.0f * p.clusterBrightness);
        stars.push_back(st);
      }

      // Young stars about them: fainter, still mostly hot, bunched within a
      // tenth of the bubble's radius, so a cluster reads as a cluster and not as
      // a few isolated beacons.
      for (int j = 0; j < p.youngPerCluster; j++) {
        Star st{};
        float local[3], sky[3], d[3];
        rng.unitVector(d);
        float r = 0.1f * cbrtf(rng.uniform());
        for (int k = 0; k < 3; k++) {
          local[k] = cl.pos[k] + d[k] * r;
        }
        bubbleToSky(b, local, sky);
        std::memcpy(st.dir, sky, sizeof(sky));
        normalize3(st.dir);
        st.distance = sqrtf(sky[0] * sky[0] + sky[1] * sky[1] + sky[2] * sky[2]);
        // Flux before temperature: the labs drew both in one call's arguments,
        // which GCC evaluates right to left, and seeds are kept compatible.
        float flux = powf(rng.range(0.02f, 1.0f), -1.0f) * 2.0f / (dist * dist);
        setFlux(st, rng.range(8000.0f, 30000.0f), flux);
        stars.push_back(st);
      }
    }
  }
}

// Uniformly within a ball of radius reach about the observer, no nearer than
// kMinDistance.
void candidate(Random& rng, float reach, float offset[3], float& d) {
  rng.unitVector(offset);
  d = fmaxf(reach * cbrtf(rng.uniform()), kMinDistance);
  offset[0] *= d;
  offset[1] *= d;
  offset[2] *= d;
}

float totalLight(const GalaxySample& s) {
  return s.old + s.young;
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
    tau += galaxyDensity(g, p).dust;
  }
  return tau * len / static_cast<float>(steps);
}

void addFieldStars(Random& rng, std::vector<Star>& stars, const Galaxy& g, const StarParams& p) {
  float offset[3], pos[3], d, most = 0.0f;
  long attempts = 0, limit = static_cast<long>(p.count) * 400;
  int placed = 0;

  // A bound on the density within reach, for rejection sampling, from a
  // scatter of samples and padded; where a sample later exceeds it the bound is
  // raised, which biases the few stars already placed by an amount nobody
  // could see.
  for (int i = 0; i < 4096; i++) {
    candidate(rng, p.reach, offset, d);
    for (int k = 0; k < 3; k++) {
      pos[k] = g.observer[k] + offset[k];
    }
    most = fmaxf(most, totalLight(galaxyDensity(g, pos)));
  }
  most *= 1.5f;
  if (most <= 0.0f) {
    return;
  }
  // So that a star of unit luminosity at a typical distance has unit flux, the
  // brightness the star dial was set up around.
  float typical = 0.7f * p.reach;

  while (placed < p.count && attempts++ < limit) {
    candidate(rng, p.reach, offset, d);
    for (int k = 0; k < 3; k++) {
      pos[k] = g.observer[k] + offset[k];
    }
    GalaxySample s = galaxyDensity(g, pos);
    float light = totalLight(s);
    if (light > most) {
      most = light;
    }
    if (rng.uniform() * most > light) {
      continue;
    }
    placed++;

    // Luminosity from a steep power law, so most stars are ordinary and a few
    // are giants; the arms' young stars are hot and several times brighter.
    bool young = rng.uniform() * light < s.young;
    float lum = fminf(powf(fmaxf(1.0f - rng.uniform(), 1e-6f), -1.0f / 1.2f), 2000.0f);
    float kelvin;
    if (young) {
      lum *= 4.0f;
      kelvin = rng.range(10000.0f, 30000.0f);
    } else {
      kelvin = starTemperature(rng, lum);
    }
    Star st{};
    // Into the sky frame: the transpose of the galaxy's rotation.
    for (int k = 0; k < 3; k++) {
      st.dir[k] = g.rot[0 + k] * offset[0] + g.rot[3 + k] * offset[1] + g.rot[6 + k] * offset[2];
    }
    normalize3(st.dir);
    st.distance = d / p.kpcPerSkyUnit;
    float tau = dustDepth(g, offset);
    float rgb[3];
    blackbody(kelvin, rgb);
    for (int k = 0; k < 3; k++) {
      st.flux[k] = rgb[k] * fminf(lum * typical * typical / (d * d), kMaxFlux) *
                   expf(-tau * p.reddening[k]);
    }
    stars.push_back(st);
  }
}

}  // namespace

void blackbody(float kelvin, float rgb[3]) {
  // Planck's law at the three wavelengths nearest the display primaries'
  // dominant ones. Crude beside the colour matching functions, but it orders
  // the colours correctly -- red dwarfs orange, the Sun faintly warm, O stars
  // blue white -- and that is all a star field needs.
  static const float lambda[3] = {610e-9f, 550e-9f, 465e-9f};
  for (int i = 0; i < 3; i++) {
    double l = lambda[i];
    rgb[i] = static_cast<float>(1.0 / (l * l * l * l * l) / (std::exp(1.4388e-2 / (l * kelvin)) - 1.0));
  }
  float lum = 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
  // Unit luminance, then a quarter of the way back to white: stars look less
  // coloured than their spectra, being at the edge of colour vision.
  for (int i = 0; i < 3; i++) {
    rgb[i] = 0.75f * rgb[i] / lum + 0.25f;
  }
}

std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p) {
  std::vector<Star> stars;
  Random rng(s.seed * 2246822519u + 101u);
  addFieldStars(rng, stars, g, p);
  addClusterStars(rng, stars, s, p);
  return stars;
}

}  // namespace starcanopy
