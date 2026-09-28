#pragma once

#include <cstdint>

namespace starcanopy {

// What the sky contains, worked out from a seed and a handful of dials, with no
// GL in it.
//
// THE ONE PRIMITIVE IS A BUBBLE: a mass of gas round a cavity blown by a
// cluster of hot young stars, as the Rosette is around NGC 2244. Where the
// viewer stands relative to one decides what it looks like:
//
//   viewer inside         the main nebula: the mass wraps the sky, lit from within
//   viewer well outside   a distant nebula some degrees across, lit from beside
//
// SKY UNITS: the main bubble's radius is 1 and the viewer is at the origin.
// The shaders treat each bubble's gas in that bubble's own frame, where it is a
// unit sphere, so form, fold, optical depth and brightness are scale free:
// a bubble twenty times further away and twenty times larger is the same object
// at the same surface brightness, as physics says. Only a texel's angular size,
// and so how many octaves of detail it can hold, depends on where it is.
//
// The cluster is NEVER at the viewer. A light at the eye lights every surface
// face on and casts no shadow the eye can see, which flattens everything;
// placing the cluster to one side is what makes the light rake across the gas.

constexpr int kMaxBubbles = 8;
// Per bubble. A bubble the size of the Gum Nebula has two lighting stars, and
// Orion-Eridanus a whole association; several is also what lets a bubble that
// wraps the sky be interesting all the way round.
constexpr int kMaxClusters = 3;
constexpr int kStarsPerCluster = 4;

struct SceneParams {
  uint32_t seed = 1;
  // The main bubble. The viewer's distance from its centre in bubble radii:
  // below 1 is inside, about 1 against the wall, above 1 outside.
  float viewerOffset = 0.55f;
  float clusterOffset = 0.85f;  // clusters' typical distance from the centre
  float luminosity = 1.0f;      // the brightest cluster's ionising output
  int clusters = 1;             // how many light the main bubble, 1..kMaxClusters
  // Shape shared by every bubble in bubble units, varied per bubble by seed.
  // The stride scale is not a shape: the march's step is a fraction of it,
  // and it sets the bubble's bound (bubbleBound()). It was look 1's shell's
  // half thickness, and keeps its value so the sky does.
  float stride = 0.06f;
  float fold = 0.18f;       // how far the fold warp displaces the gas
  float keep = 0.7f;        // how much of the gas survives the holes, 0..1
  // The more distant nebulae: apparent radius drawn between the two angles,
  // distance whatever follows from that and a size like the main one's.
  int distantCount = 3;
  float distantMinDegrees = 3.0f, distantMaxDegrees = 14.0f;
};

struct ClusterStar {
  float dir[3];     // unit vector from the viewer
  float intensity;  // already divided by distance squared
};

struct Cluster {
  float pos[3];      // in the bubble's own unit frame
  float luminosity;  // zero for an unused slot
  ClusterStar star[kStarsPerCluster];
};

struct Bubble {
  float center[3];  // sky units
  float radius;     // sky units
  // Sky-frame offset into the bubble's frame, row major. Each bubble's own
  // orientation to the shared noise volume, so no two read the same pattern.
  float rot[9];
  float stride, fold, keep;
  float noiseOffset;  // where in the tiling noise this bubble reads
  // How much the bubble is squeezed along each of its axes, each 1 or more: an
  // ellipsoid inside the unit sphere, so every bound on the bubble still
  // holds. The main bubble is round; distant ones are not.
  float squeeze[3];
  float density;  // multiplies the gas
  // What of its light reaches the viewer through the dust in front of it, per
  // channel: 1 for the main bubble, less and redder for one far off behind
  // the galaxy's dust. Set once the galaxy is known (sky.cpp); 1 until then.
  float veil[3];
  Cluster cluster[kMaxClusters];
};

struct Scene {
  uint32_t seed;
  int bubbleCount;  // bubble[0] is the main one
  Bubble bubble[kMaxBubbles];
};

// Deterministic in the parameters: the same params always make the same scene.
Scene generateScene(const SceneParams& p);

// A point in a bubble's own frame, into sky units.
void bubbleToSky(const Bubble& b, const float local[3], float out[3]);

// How far past its unit radius a bubble's gas can reach, in bubble radii. The
// ray bounds and the light volume's box are both this; too small clips the
// outer folds flat.
float bubbleBound(const Bubble& b);

}  // namespace starcanopy
