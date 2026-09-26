#pragma once

#include "galaxy.h"
#include "scene.h"

#include <vector>

namespace starcanopy {

// The sky's stars, as points at real distances, so that the nebula can be in
// front of some and behind others.
//
// A star painted into a texture before the nebula existed can only be dimmed
// uniformly by all the gas on its line of sight; it has no distance, so it
// cannot be in front of the shell, or in it. These carry one, and the bake
// looks up how much gas lies between the viewer and exactly that distance: a
// star in front of the near wall is untouched, one behind a dark lane vanishes,
// one seen through a thin edge is dimmed and reddened by exactly that edge.
//
// The field stars are drawn from the galaxy itself: positions from its density
// within `reach` of the observer, so they thin out away from the disc and
// toward the galaxy's edge exactly as its glow does, the arms' young stars hot
// and luminous among them. They are the brightest few thousand of a population
// whose faint members are the glow, a negligible share of its light, so the
// glow counts every distance and nothing is noticeably counted twice. Each is
// dimmed and reddened by the galaxy's dust between it and the observer.
//
// Brightness is a luminosity from a power law over the distance squared, and
// the distance is carried into the nebula's sky units, so the few stars nearer
// than the nebula are in front of it -- and, being nearest, mostly the
// brightest, as the brightest stars of any real sky are its neighbours.
//
// Then the clusters: their few brilliant lighting stars and fainter young ones.

struct StarParams {
  int count = 30000;              // field stars
  float reach = 1.5f;             // kpc: how far out stars are drawn as points
  float kpcPerSkyUnit = 0.08f;    // the nebula's scale: its main bubble's radius
  float reddening[3] = {0.8f, 1.0f, 1.3f};  // the galaxy's dust law
  int youngPerCluster = 40;
  float clusterBrightness = 1.0f;  // the lighting stars, relative to their luminosity
};

struct Star {
  float dir[3];
  float distance;  // sky units
  float flux[3];   // linear rgb, after the galaxy's dust
};

// Deterministic in the scene's seed and the params.
std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p);

// A black body's colour, as linear rgb normalised to unit luminance, then a
// quarter of the way back to white.
void blackbody(float kelvin, float rgb[3]);

}  // namespace starcanopy
