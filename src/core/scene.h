#pragma once

#include <cstdint>

namespace starcanopy {

// What the sky contains, worked out from a seed and a handful of dials, with no
// GL in it.
//
// THE ONE PRIMITIVE IS A BUBBLE: a shell of gas blown by a cluster of hot young
// stars in its cavity, as the Rosette is around NGC 2244. Where the viewer
// stands relative to one decides what it looks like, so one model covers every
// case the sky needs:
//
//   viewer well inside    the shell wraps the whole sky, near wall and far wall
//   viewer near the wall  a vast curved front filling half the sky or more
//   viewer well outside   a distant nebula, a ring or a lobe some degrees across
//
// SKY UNITS: the main bubble's radius is 1 and the viewer is at the origin.
// The shaders treat each bubble's gas in that bubble's own frame, where it is a
// unit sphere, so thickness, fold, optical depth and brightness are scale free:
// a bubble twenty times further away and twenty times larger is the same object
// at the same surface brightness, as physics says. Only a texel's angular size,
// and so how many octaves of detail it can hold, depends on where it is.
//
// The cluster is NEVER at the viewer. A light at the eye lights every surface
// face on and casts no shadow the eye can see, which flattens everything;
// placing the cluster to one side is what makes the light rake across the gas.

constexpr int kMaxBubbles = 8;
// Per bubble. A shell the size of the Gum Nebula has two lighting stars, and
// Orion-Eridanus a whole association; several is also what lets a bubble that
// wraps the sky be interesting all the way round.
constexpr int kMaxClusters = 3;
// Capsules across the whole sky -- pillars, and the segments of the dark clouds
// -- of which the main bubble takes most. Must match NSKY_MAX_PILLARS in
// field.glsl.
constexpr int kMaxPillars = 56;
constexpr int kStarsPerCluster = 4;

struct SceneParams {
  uint32_t seed = 1;
  // The main bubble. The viewer's distance from its centre in bubble radii:
  // below 1 is inside, about 1 against the wall, above 1 outside.
  float viewerOffset = 0.55f;
  float clusterOffset = 0.85f;  // clusters' typical distance from the centre
  float luminosity = 1.0f;      // the brightest cluster's ionising output
  int clusters = 2;             // how many light the main bubble, 1..kMaxClusters
  // Pillars on the main bubble's wall, and their size in bubble radii. Distant
  // bubbles get none: a few degrees across, a pillar's dark core is a blot.
  int pillars = 9;
  float pillarLength = 0.3f, pillarWidth = 0.045f;
  // Dark clouds adrift in the main bubble's cavity: how many, how long and
  // thick, and how far from the viewer at most, in bubble radii.
  int clouds = 4;
  float cloudLength = 0.15f, cloudWidth = 0.022f, cloudDistance = 0.35f;
  // Shape shared by every bubble in bubble units, varied per bubble by seed.
  float thickness = 0.06f;  // the shell's half thickness
  float fold = 0.18f;       // how far the fold warp displaces the shell
  float keep = 0.7f;        // how much of the shell survives the holes, 0..1
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

// An elephant trunk: a column of dense dusty gas standing off the shell's inner
// face, pointing at the cluster that lights it. A tapered capsule from a base
// buried in the wall to a rounded tip, in the bubble's WARPED frame -- the one
// the shell is a unit sphere in -- so it bends with the folds and stays rooted
// where the wall actually is.
struct Pillar {
  float base[3], baseRadius;
  float tip[3], tipRadius;
  // Not rooted: a segment of a dark cloud adrift in the cavity, placed in the
  // UNWARPED frame, having no wall to follow; the fold warp is large enough to
  // carry a cloud meant to be near the viewer onto the camera.
  bool adrift;
};

struct Bubble {
  float center[3];  // sky units
  float radius;     // sky units
  // Sky-frame offset into the bubble's frame, row major. Each bubble's own
  // orientation to the shared noise volume, so no two read the same pattern.
  float rot[9];
  float thickness, fold, keep;
  float noiseOffset;  // where in the tiling noise this bubble reads
  // How much the shell is squeezed along each of its axes, each 1 or more: an
  // ellipsoid inside the unit sphere, so every bound on the bubble still holds.
  // Only a bubble without pillars may be squeezed; they sit on the unit sphere.
  // And how hard its outer edge is, relative to the dial. The main bubble is
  // round; distant ones are not, since a round shell seen from outside is a
  // disc with a hard circular rim -- a coin stuck on the sky.
  float squeeze[3];
  float edge;
  float density;  // multiplies the gas, so a thicker shell keeps its column
  Cluster cluster[kMaxClusters];
  int firstPillar, pillarCount;  // a range of the scene's pillars
};

struct Scene {
  uint32_t seed;
  int bubbleCount;  // bubble[0] is the main one
  Bubble bubble[kMaxBubbles];
  int pillarCount;
  Pillar pillar[kMaxPillars];
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
