#include "accents.h"

#include "random.h"

#include <cmath>

namespace starcanopy {

namespace {

// kpc: no nearer than an association's own size, no further than the far side
// of the arms seen along the disc.
constexpr float kNearest = 0.05f, kFarthest = 8.0f;
// Where the reference count is set: a place in the band, as the seed's own
// place is (observerPlace()).
constexpr float kReferenceRadius = 2.6f;

// Poisson by multiplication: a sky has a handful.
int poisson(Random& rng, float mean) {
  float l = expf(-mean), prod = rng.uniform();
  int n = 0;
  while (prod > l && n < 1000) {
    n++;
    prod *= rng.uniform();
  }
  return n;
}

// Where open clusters are born: the young disc, with a little of the old, so a
// galaxy with next to no arms -- a lenticular -- still has a few.
float birthplace(const GalaxySample& s) {
  return s.young + 0.05f * s.old;
}

// A direction uniformly, and a distance with density d^k -- d^2 the volume's
// own, so k = 2 is physical and lower tilts the draw toward the near; at
// k = -1, even in log distance, as many within 0.6 kpc as beyond.
void propose(Random& rng, float k, float offset[3], float& d) {
  rng.unitVector(offset);
  float e = k + 1.0f;
  if (fabsf(e) < 1e-3f) {
    d = kNearest * powf(kFarthest / kNearest, rng.uniform());
  } else {
    float a = powf(kNearest, e), b = powf(kFarthest, e);
    d = powf(a + rng.uniform() * (b - a), 1.0f / e);
  }
  for (int i = 0; i < 3; i++) {
    offset[i] *= d;
  }
}

// The mean birthplace density about a place, under the same draw: how much
// young disc lies within reach of it, as the near bias weighs it.
double reach(const Galaxy& g, const float from[3], float k) {
  Random scatter(0x2545f491u);
  double sum = 0.0;
  for (int i = 0; i < 2048; i++) {
    float offset[3], d, pos[3];
    propose(scatter, k, offset, d);
    for (int j = 0; j < 3; j++) {
      pos[j] = from[j] + offset[j];
    }
    sum += birthplace(galaxyDensity(g, pos, 0.1f));
  }
  return sum / 2048.0;
}

}  // namespace

std::vector<Accent> generateAccents(uint32_t seed, const Galaxy& g, const AccentParams& p) {
  std::vector<Accent> out;
  if (p.openClusters <= 0) {
    return out;
  }
  Random rng(seed * 2862933555u + 3037u);
  // 0 physical (k = 2), 0.75 even in log distance (k = -1), 1 strongly near
  // (k = -2): only near clusters stand out -- their members among the
  // brightest stars about them -- and a draw merely even in distance still
  // put most kiloparsecs off in the band, lost among its stars.
  float k = 2.0f - 4.0f * fminf(fmaxf(p.near, 0.0f), 1.0f);

  // As many as the dial asks about a place in the band, scaled by how much
  // young disc lies within reach here: more in toward the centre, few far out,
  // and none conjured in the empty halo.
  float r = sqrtf(g.observer[0] * g.observer[0] + g.observer[1] * g.observer[1]);
  float reference[3] = {kReferenceRadius * g.scaleLength, 0.0f, 0.0f};
  if (r > 0.0f) {
    reference[0] = g.observer[0] / r * kReferenceRadius * g.scaleLength;
    reference[1] = g.observer[1] / r * kReferenceRadius * g.scaleLength;
  }
  double here = reach(g, g.observer, k), there = reach(g, reference, k);
  float share = there > 0.0 ? static_cast<float>(fmin(here / there, 3.0)) : 0.0f;
  int n = poisson(rng, static_cast<float>(p.openClusters) * share);
  if (n == 0) {
    return out;
  }

  // Rejection against a bound from a scatter of samples, padded; raised where a
  // sample exceeds it, as the field stars' is.
  float offset[3], pos[3], d, most = 0.0f;
  for (int i = 0; i < 4096; i++) {
    propose(rng, k, offset, d);
    for (int j = 0; j < 3; j++) {
      pos[j] = g.observer[j] + offset[j];
    }
    most = fmaxf(most, birthplace(galaxyDensity(g, pos, 0.1f)));
  }
  most *= 1.5f;
  if (most <= 0.0f) {
    return out;
  }
  long attempts = 0;
  while (static_cast<int>(out.size()) < n && attempts++ < 200000L) {
    propose(rng, k, offset, d);
    for (int j = 0; j < 3; j++) {
      pos[j] = g.observer[j] + offset[j];
    }
    float w = birthplace(galaxyDensity(g, pos, 0.1f));
    most = fmaxf(most, w);
    if (rng.uniform() * most > w) {
      continue;
    }
    Accent a{};
    a.kind = kOpenCluster;
    // Associations -- loose young groups tens of parsecs across -- are parked:
    // seen from near by they dissolve into the field, and an accent not seen
    // is not worth having (docs/studies/accents.md). The draw stays, so the
    // clusters after are as they were.
    a.association = rng.uniform() < 0.0f;
    // A dense core of a few parsecs, as real open clusters have: at 3-12 pc
    // their stars were too thinly spread to read as one object, and at 1.5-5
    // a cluster a kiloparsec off was a speck.
    a.radius = a.association ? rng.range(0.02f, 0.06f) : rng.range(0.0025f, 0.008f);
    a.seed = static_cast<uint32_t>(rng.below(0x7fffffff));
    a.distance = d;
    for (int j = 0; j < 3; j++) {
      a.pos[j] = pos[j];
      a.offset[j] = offset[j];
    }
    // Into the sky frame: the transpose of the galaxy's rotation.
    for (int j = 0; j < 3; j++) {
      a.dir[j] = (g.rot[0 + j] * offset[0] + g.rot[3 + j] * offset[1] + g.rot[6 + j] * offset[2]) / d;
    }
    out.push_back(a);
  }
  return out;
}

}  // namespace starcanopy
