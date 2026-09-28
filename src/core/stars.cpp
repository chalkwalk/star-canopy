#include "stars.h"

#include "random.h"
#include "sample.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

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

// The galaxy's mean light in a ball of radius reach about centre, from a
// fixed scatter of points so it is the same every time.
double meanLight(const Galaxy& g, const float centre[3], float reach) {
  Random scatter(0x9e3779b9u);
  double light = 0.0;
  const int samples = 4096;
  for (int i = 0; i < samples; i++) {
    float offset[3], d, pos[3];
    candidate(scatter, reach, offset, d);
    for (int k = 0; k < 3; k++) {
      pos[k] = centre[k] + offset[k];
    }
    light += totalLight(galaxyDensity(g, pos, reach));
  }
  return light / samples;
}

// How many field stars: p.count about a place on the midplane, and as many
// fewer as the galaxy's light about the observer is less than there -- above
// the disc, the ball of reach is mostly empty. Measured against the midplane
// under the observer, so the band's density (addBandStars) carries on from the
// same scale wherever the observer is.
double midplaneLight(const Galaxy& g, float reach) {
  const float below[3] = {g.observer[0], g.observer[1], 0.0f};
  return meanLight(g, below, reach);
}

void addFieldStars(Random& rng, std::vector<Star>& stars, const Galaxy& g, const StarParams& p) {
  float offset[3], pos[3], d, most = 0.0f;
  // At the midplane, p.count; away from it, as many as its light allows.
  double here = meanLight(g, g.observer, p.reach), there = midplaneLight(g, p.reach);
  int count = g.observer[2] == 0.0f || there <= 0.0
                  ? p.count
                  : static_cast<int>(p.count * std::min(here / there, 1.0));
  long attempts = 0, limit = static_cast<long>(count) * 400;
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

  while (placed < count && attempts++ < limit) {
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

// The luminosity function the field stars are drawn from, a power law of
// index 1.2 from 1 to kMaxLum: the share of stars brighter than x.
constexpr float kLumIndex = 1.2f, kMaxLum = 2000.0f;

float brighterThan(float x) {
  if (x <= 1.0f) {
    return 1.0f;
  }
  if (x >= kMaxLum) {
    return 0.0f;
  }
  return (powf(x, -kLumIndex) - powf(kMaxLum, -kLumIndex)) / (1.0f - powf(kMaxLum, -kLumIndex));
}

// The band: the galaxy beyond the field stars' reach, as stars. For each patch
// of sky -- kBandCells a side on each cube face -- the expected number of stars
// brighter than a limit is counted along its middle, out through the density
// and its dust; each is then drawn somewhere in its patch, at a distance and a
// luminosity the count weighs, and dimmed by the dust along its own line of
// sight, so the dark lanes lose their stars as finely as the dust is drawn.
// The limit is set so the band has about p.bandCount stars; the stars fainter
// than it stay the galaxy's glow (galaxy.shader). The density is scaled to the
// field stars', so the band carries on from them. Returns the limit, as flux.
constexpr int kBandCells = 48, kBandSteps = 64;

float addBandStars(Random& rng, std::vector<Star>& stars, const Galaxy& g, const StarParams& p) {
  if (p.bandCount <= 0 || p.count <= 0) {
    return 0.0f;
  }
  const float pi = 3.14159265f;
  // Stars per unit of light per cubic kpc, from the field stars: p.count of
  // them in a ball of their reach on the midplane under the observer -- about
  // the observer itself, one above the disc would find the ball empty and make
  // every star of the band blinding.
  double perLight =
      p.count / (midplaneLight(g, p.reach) * 4.0 / 3.0 * pi * p.reach * p.reach * p.reach);
  float typical = 0.7f * p.reach, typical2 = typical * typical;
  float end = 1.5f * g.edge;

  // Along each patch's middle: at each step its distance, what of its light
  // the dust in front lets through, and its old and young light, so the counts
  // can be weighed for any limit.
  struct Step {
    float t, dt, transmit, old, young;
  };
  const int cells = 6 * kBandCells * kBandCells;
  std::vector<Step> steps(static_cast<size_t>(cells) * kBandSteps);
  float cellAngle = (2.0f / kBandCells) / 1.2f;  // radians, about
  for (int c = 0; c < cells; c++) {
    int face = c / (kBandCells * kBandCells), cell = c % (kBandCells * kBandCells);
    float s = ((cell % kBandCells) + 0.5f) / kBandCells * 2.0f - 1.0f;
    float tt = ((cell / kBandCells) + 0.5f) / kBandCells * 2.0f - 1.0f;
    float dir[3], d[3];
    faceDirection(face, s, tt, dir);
    normalize3(dir);
    galaxyToFrame(g, dir, d);
    // Out to where the ray leaves the galaxy's bound, as the glow's march.
    float b = dot3(g.observer, d), cc = dot3(g.observer, g.observer) - end * end;
    float far = fmaxf(-b + sqrtf(fmaxf(b * b - cc, 0.0f)), p.reach);
    float tau = 0.0f;
    for (int i = 0; i < kBandSteps; i++) {
      Step& st = steps[static_cast<size_t>(c) * kBandSteps + i];
      float ta = p.reach * powf(far / p.reach, static_cast<float>(i) / kBandSteps);
      float tb = p.reach * powf(far / p.reach, static_cast<float>(i + 1) / kBandSteps);
      st.t = 0.5f * (ta + tb);
      st.dt = tb - ta;
      float pos[3];
      for (int k = 0; k < 3; k++) {
        pos[k] = g.observer[k] + d[k] * st.t;
      }
      GalaxySample gs = galaxyDensity(g, pos, fmaxf(st.dt, cellAngle * st.t));
      st.transmit = expf(-tau);
      st.old = gs.old;
      st.young = gs.young;
      tau += gs.dust * st.dt;
    }
  }
  float omega = 4.0f * pi / cells;
  // How many stars a patch's step holds brighter than the limit: the old
  // brighter than x in luminosity, the young -- four times as luminous, as the
  // field stars' are -- brighter than x / 4.
  auto count = [&](const Step& st, float limit, float& oldPart, float& youngPart) {
    float x = limit * st.t * st.t / (typical2 * fmaxf(st.transmit, 1e-6f));
    float volume = static_cast<float>(perLight) * st.t * st.t * st.dt * omega;
    oldPart = volume * st.old * brighterThan(x);
    youngPart = volume * st.young * brighterThan(x / 4.0f);
    return oldPart + youngPart;
  };
  // The limit for the budget: the count at a first guess, the guess moved by
  // the power law's slope, and again.
  float limit = 1.0f;
  for (int pass = 0; pass < 4; pass++) {
    double total = 0.0;
    for (const Step& st : steps) {
      float o, y;
      total += count(st, limit, o, y);
    }
    if (total <= 0.0) {
      return 0.0f;
    }
    limit *= powf(static_cast<float>(total / p.bandCount), 1.0f / kLumIndex);
  }

  std::vector<float> weight(kBandSteps);
  for (int c = 0; c < cells; c++) {
    const Step* col = &steps[static_cast<size_t>(c) * kBandSteps];
    float lambda = 0.0f;
    for (int i = 0; i < kBandSteps; i++) {
      float o, y;
      weight[i] = count(col[i], limit, o, y);
      lambda += weight[i];
    }
    // As many as the count, by a draw: Poisson for the few, near enough
    // normal for the many.
    int n;
    if (lambda < 30.0f) {
      float l = expf(-lambda), prod = rng.uniform();
      n = 0;
      while (prod > l) {
        n++;
        prod *= rng.uniform();
      }
    } else {
      float u1 = fmaxf(rng.uniform(), 1e-7f), u2 = rng.uniform();
      float z = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * pi * u2);
      n = std::max(0, static_cast<int>(lambda + sqrtf(lambda) * z + 0.5f));
    }
    int face = c / (kBandCells * kBandCells), cell = c % (kBandCells * kBandCells);
    for (int j = 0; j < n; j++) {
      // Where about the patch: a tent two patches wide about its middle, not
      // evenly within it, so a patch's count blends into its neighbours' --
      // evenly, each patch's count is a step, and where the band's density
      // changes fast, at its edges, the steps show as a saw.
      float s = ((cell % kBandCells) + 0.5f + rng.uniform() + rng.uniform() - 1.0f) / kBandCells *
                    2.0f - 1.0f;
      float tt = ((cell / kBandCells) + 0.5f + rng.uniform() + rng.uniform() - 1.0f) / kBandCells *
                     2.0f - 1.0f;
      // And at which step, as the count weighs them.
      float u = rng.uniform() * lambda;
      int i = 0;
      while (i < kBandSteps - 1 && u > weight[i]) {
        u -= weight[i];
        i++;
      }
      const Step& st = col[i];
      float t = st.t + (rng.uniform() - 0.5f) * st.dt;
      float o, y;
      count(st, limit, o, y);
      bool young = rng.uniform() * (o + y) < y;
      // A luminosity above what the limit asks at that distance, from the
      // power law cut there.
      float x = limit * t * t / (typical2 * fmaxf(st.transmit, 1e-6f)) / (young ? 4.0f : 1.0f);
      float a = powf(fmaxf(x, 1.0f), -kLumIndex), top = powf(kMaxLum, -kLumIndex);
      float lum = powf(fmaxf(a - rng.uniform() * (a - top), top), -1.0f / kLumIndex) *
                  (young ? 4.0f : 1.0f);
      float kelvin = young ? rng.range(10000.0f, 30000.0f) : starTemperature(rng, lum);
      Star star{};
      faceDirection(face, s, tt, star.dir);
      normalize3(star.dir);
      float gd[3], offset[3];
      galaxyToFrame(g, star.dir, gd);
      for (int k = 0; k < 3; k++) {
        offset[k] = gd[k] * t;
      }
      // Dimmed by the dust along its own line, not its patch's middle.
      float tauStar = dustDepth(g, offset);
      float rgb[3];
      blackbody(kelvin, rgb);
      float bright = fminf(lum * typical2 / (t * t), kMaxFlux);
      for (int k = 0; k < 3; k++) {
        star.flux[k] = rgb[k] * bright * expf(-tauStar * p.reddening[k]);
      }
      star.distance = t / p.kpcPerSkyUnit;
      stars.push_back(star);
    }
  }
  return limit;
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

std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p,
                                float* bandFlux) {
  std::vector<Star> stars;
  Random rng(s.seed * 2246822519u + 101u);
  addFieldStars(rng, stars, g, p);
  addClusterStars(rng, stars, s, p);
  // From a stream of their own, so the stars above are as they were.
  Random band(s.seed * 1597334677u + 409u);
  float limit = addBandStars(band, stars, g, p);
  if (bandFlux) {
    *bandFlux = limit;
  }
  return stars;
}

}  // namespace starcanopy
