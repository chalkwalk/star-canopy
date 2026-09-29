#pragma once

#include "galaxy.h"

#include <cstdint>
#include <vector>

namespace starcanopy {

// The sky's accents: objects beyond the lit mass and the galaxy -- open
// clusters first; globular clusters, glowing shells and the rest to come
// (ROADMAP.md, Astronomical objects). Placed where the galaxy puts them, at
// physical distances, the draw tilted toward the near by accent-near: seen
// from anywhere most such objects are small, and a sky wants a few near
// enough to read, never one enlarged past what its distance allows
// (PRINCIPLES §4).

enum AccentKind { kOpenCluster };

// The most accents whose unresolved light the galaxy's glow carries.
constexpr int kMaxAccentGlows = 16;

struct AccentParams {
  int openClusters = 0;  // typical count about a place in the band; 0 none
  float near = 0.4f;     // 0 as physics has them, 1 strongly favouring the near
};

struct Accent {
  int kind;
  bool association;   // the loose end: large, few, young
  float pos[3];       // kpc, galaxy frame
  float offset[3];    // kpc, galaxy frame, from the observer
  float dir[3];       // unit, sky frame
  float distance;     // kpc
  float radius;       // kpc: how far its lumps spread
  uint32_t seed;      // for its details, from a stream of its own
  // Filled by generateStars(): the light of the members too faint to draw,
  // before the dust in front, per channel, in star-flux units.
  float glow[3];
};

// Deterministic in the seed, the galaxy and the params, from a stream of its
// own, so no other draw moves.
std::vector<Accent> generateAccents(uint32_t seed, const Galaxy& g, const AccentParams& p);

}  // namespace starcanopy
