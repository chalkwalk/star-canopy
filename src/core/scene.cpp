#include "scene.h"

#include "random.h"

#include <cmath>
#include <cstring>

namespace starcanopy {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// An orthonormal basis from two random directions, Gram-Schmidt. Rows of the
// result are the bubble's axes in the sky frame.
void randomRotation(Random& rng, float m[9]) {
  float a[3], b[3], c[3], d;
  rng.unitVector(a);
  do {
    rng.unitVector(b);
    d = dot3(a, b);
  } while (fabsf(d) > 0.95f);
  b[0] -= d * a[0];
  b[1] -= d * a[1];
  b[2] -= d * a[2];
  normalize3(b);
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
  std::memcpy(&m[0], a, sizeof(a));
  std::memcpy(&m[3], b, sizeof(b));
  std::memcpy(&m[6], c, sizeof(c));
}

void skyToBubble(const Bubble& b, const float sky[3], float out[3]) {
  float d[3];
  for (int i = 0; i < 3; i++) {
    d[i] = (sky[i] - b.center[i]) / b.radius;
  }
  for (int i = 0; i < 3; i++) {
    out[i] = b.rot[i * 3 + 0] * d[0] + b.rot[i * 3 + 1] * d[1] + b.rot[i * 3 + 2] * d[2];
  }
}

// A cluster in the cavity, clear of the viewer and of the clusters already
// placed. One next to the eye is the headlamp again, every wall lit face on;
// two on top of each other are one cluster, and the point of several is that
// they light different parts of the sky. Distance is what matters, not
// direction, so a position is redrawn until it is far enough from both.
void placeCluster(Random& rng, Bubble& b, int index, float offset) {
  static const float origin[3] = {0.0f, 0.0f, 0.0f};
  Cluster& c = b.cluster[index];
  float viewer[3], dir[3], d[3];
  skyToBubble(b, origin, viewer);
  for (int tries = 0; tries < 64; tries++) {
    rng.unitVector(dir);
    for (int j = 0; j < 3; j++) {
      c.pos[j] = dir[j] * offset;
    }
    for (int j = 0; j < 3; j++) {
      d[j] = c.pos[j] - viewer[j];
    }
    bool ok = sqrtf(dot3(d, d)) > 0.4f;
    for (int i = 0; i < index && ok; i++) {
      for (int j = 0; j < 3; j++) {
        d[j] = c.pos[j] - b.cluster[i].pos[j];
      }
      ok = sqrtf(dot3(d, d)) > 0.6f * offset + 0.2f;
    }
    if (ok) {
      break;
    }
  }
}

// A handful of hot stars about a cluster's centre, so the light has a visible
// source. Brightness shared out steeply -- one or two dominate, as in every
// real young cluster -- then dimmed by distance, so a far bubble's stars are
// fainter than the main one's.
void placeStars(Random& rng, const Bubble& b, Cluster& c) {
  float weight[kStarsPerCluster], total = 0.0f;
  for (int i = 0; i < kStarsPerCluster; i++) {
    weight[i] = powf(rng.range(0.02f, 1.0f), 3.0f);
    total += weight[i];
  }
  for (int i = 0; i < kStarsPerCluster; i++) {
    ClusterStar& s = c.star[i];
    float local[3], sky[3], dir[3];
    rng.unitVector(dir);
    for (int j = 0; j < 3; j++) {
      local[j] = c.pos[j] + dir[j] * 0.05f * cbrtf(rng.uniform());
    }
    bubbleToSky(b, local, sky);
    float d2 = dot3(sky, sky);
    for (int j = 0; j < 3; j++) {
      s.dir[j] = sky[j];
    }
    normalize3(s.dir);
    s.intensity = c.luminosity * weight[i] / total / fmaxf(d2, 1e-4f);
  }
}

// The first cluster is the brightest; the others lesser groups, as an
// association's subgroups are.
void lightBubble(Random& rng, Bubble& b, int n, float offset, float luminosity) {
  for (int i = 0; i < n; i++) {
    b.cluster[i].luminosity = luminosity * (i == 0 ? 1.0f : rng.range(0.3f, 0.8f));
    placeCluster(rng, b, i, offset * (i == 0 ? 1.0f : rng.range(0.7f, 1.3f)));
    placeStars(rng, b, b.cluster[i]);
  }
}

// Pillars come in groups -- the Eagle's are three abreast -- on the stretch of
// wall facing a cluster, each pointing at it; the brighter the cluster, the
// likelier it has them.
//
// From a stream of their own, one per bubble: drawn from the scene's, a change
// to the number of pillars would move every bubble placed after them.
void raisePillars(uint32_t seed, int index, Scene& s, Bubble& b, int want, const SceneParams& p) {
  Random rng(seed * 1597334677u + 911u * static_cast<uint32_t>(index + 1));
  static const float origin[3] = {0.0f, 0.0f, 0.0f};
  float viewer[3];
  float total = 0.0f, groupDir[3] = {0.0f, 0.0f, 1.0f}, cdir[3], toCluster[3], d[3];
  int chosen = 0, inGroup = 0;

  b.firstPillar = s.pillarCount;
  b.pillarCount = 0;
  skyToBubble(b, origin, viewer);
  for (int c = 0; c < kMaxClusters; c++) {
    total += b.cluster[c].luminosity;
  }
  if (total <= 0.0f) {
    return;
  }
  for (int i = 0; i < want && s.pillarCount < kMaxPillars; i++) {
    Pillar& pl = s.pillar[s.pillarCount];
    if (inGroup == 0) {
      float u = rng.uniform() * total;
      for (chosen = 0; chosen < kMaxClusters - 1; chosen++) {
        u -= b.cluster[chosen].luminosity;
        if (u <= 0.0f) {
          break;
        }
      }
      // Where on the wall. A pillar points at its cluster, often between the
      // viewer and the far wall, so one placed merely where the light falls
      // tends to point at the eye and is seen end on, as a blob; the Eagle
      // reads as pillars because its stars are off to one side of our line of
      // sight. So of two dozen sites, the one whose pillar is most nearly side
      // on, weighted toward the cluster, where the light is.
      float best = -1.0f;
      for (int tries = 0; tries < 24; tries++) {
        float site[3], axis[3], sight[3];
        rng.unitVector(site);
        for (int k = 0; k < 3; k++) {
          axis[k] = b.cluster[chosen].pos[k] - site[k];
          sight[k] = site[k] - viewer[k];
        }
        float dc = sqrtf(dot3(axis, axis));
        normalize3(axis);
        normalize3(sight);
        cdir[0] = axis[1] * sight[2] - axis[2] * sight[1];
        cdir[1] = axis[2] * sight[0] - axis[0] * sight[2];
        cdir[2] = axis[0] * sight[1] - axis[1] * sight[0];
        float sine = sqrtf(dot3(cdir, cdir));
        float score = sine * sine / (dc * dc + 0.1f);
        if (score > best) {
          best = score;
          std::memcpy(groupDir, site, sizeof(site));
        }
      }
      inGroup = 2 + rng.below(3);
    }
    inGroup--;
    const float* cl = b.cluster[chosen].pos;
    rng.unitVector(d);
    for (int k = 0; k < 3; k++) {
      pl.base[k] = groupDir[k] + 0.12f * d[k];
    }
    normalize3(pl.base);
    // Buried a little in the wall, so the root never shows.
    for (int k = 0; k < 3; k++) {
      pl.base[k] *= 1.02f;
    }
    for (int k = 0; k < 3; k++) {
      toCluster[k] = cl[k] - pl.base[k];
    }
    float len = sqrtf(dot3(toCluster, toCluster));
    if (len < 1e-4f) {
      continue;
    }
    float scale = p.pillarLength * rng.range(0.5f, 1.4f);
    for (int k = 0; k < 3; k++) {
      pl.tip[k] = pl.base[k] + toCluster[k] / len * fminf(scale, 0.8f * len);
    }
    pl.baseRadius = p.pillarWidth * rng.range(0.8f, 1.6f);
    pl.tipRadius = pl.baseRadius * rng.range(0.4f, 0.7f);
    s.pillarCount++;
    b.pillarCount++;
  }
}

float segmentDistance(const float x[3], const float a[3], const float b[3]) {
  float ab[3], ax[3], d[3];
  for (int k = 0; k < 3; k++) {
    ab[k] = b[k] - a[k];
    ax[k] = x[k] - a[k];
  }
  float len2 = dot3(ab, ab);
  float h = len2 > 0.0f ? dot3(ax, ab) / len2 : 0.0f;
  h = h < 0.0f ? 0.0f : (h > 1.0f ? 1.0f : h);
  for (int k = 0; k < 3; k++) {
    d[k] = ax[k] - ab[k] * h;
  }
  return sqrtf(dot3(d, d));
}

// Dark clouds adrift in the cavity: cold molecular gas the clusters have not
// yet dispersed, silhouetted against the glow with a bright rim where they face
// a cluster -- the Rosette's globules, the Horsehead. Each is a short curved
// chain of capsules laid across the line of sight, so it reads as a filament
// and not end on; a few are one short capsule, a globule.
//
// Each goes between the viewer and a cluster, a little off the line: BACKLIT.
// A cloud lit from the viewer's side shows its lit face and reads as one more
// bright cloud; one lit from behind shows its dark side against the brightest
// part of the sky with the light breaking round its edges -- the silhouette,
// the Horsehead's whole look. Kept clear of the viewer, so none swallows the
// camera, and of the clusters, which would have evaporated anything so close.
void setCloudsAdrift(uint32_t seed, Scene& s, Bubble& b, const SceneParams& p) {
  Random rng(seed * 3812015801u + 4099u);
  static const float origin[3] = {0.0f, 0.0f, 0.0f};
  float viewer[3];
  skyToBubble(b, origin, viewer);
  for (int i = 0; i < p.clouds; i++) {
    float sight[3], across[3], node[6][3], radius[6], centre[3], t;
    int nodes = rng.uniform() < 0.3f ? 2 : 3 + rng.below(3);
    bool ok = false;

    if (s.pillarCount + nodes - 1 > kMaxPillars) {
      break;
    }
    for (int tries = 0; tries < 32 && !ok; tries++) {
      float toward[3], off[3];
      int chosen = rng.below(kMaxClusters);
      if (b.cluster[chosen].luminosity <= 0.0f) {
        chosen = 0;
      }
      for (int k = 0; k < 3; k++) {
        toward[k] = b.cluster[chosen].pos[k] - viewer[k];
      }
      float reach = sqrtf(dot3(toward, toward));
      normalize3(toward);
      // Ten to thirty-five degrees off the line to the cluster.
      rng.perpendicular(toward, off);
      t = tanf(rng.range(10.0f, 35.0f) * kPi / 180.0f);
      for (int k = 0; k < 3; k++) {
        sight[k] = toward[k] + t * off[k];
      }
      normalize3(sight);
      t = rng.range(0.4f, 1.0f) * fminf(p.cloudDistance, 0.7f * reach);
      for (int k = 0; k < 3; k++) {
        centre[k] = viewer[k] + sight[k] * t;
      }
      ok = sqrtf(dot3(centre, centre)) < 0.8f;
      for (int c = 0; c < kMaxClusters && ok; c++) {
        float dc[3];
        if (b.cluster[c].luminosity <= 0.0f) {
          continue;
        }
        for (int k = 0; k < 3; k++) {
          dc[k] = centre[k] - b.cluster[c].pos[k];
        }
        ok = sqrtf(dot3(dc, dc)) > 0.25f;
      }
    }
    if (!ok) {
      continue;
    }

    // A walk across the line of sight, turning a little at each node.
    rng.perpendicular(sight, across);
    float seg = p.cloudLength * rng.range(0.6f, 1.4f) / static_cast<float>(nodes - 1);
    if (nodes == 2) {
      seg *= 0.25f;
    }
    std::memcpy(node[0], centre, sizeof(centre));
    for (int j = 1; j < nodes; j++) {
      float turn[3];
      rng.unitVector(turn);
      for (int k = 0; k < 3; k++) {
        across[k] += 0.5f * turn[k];
      }
      // Stay across the line of sight, not along it.
      t = dot3(across, sight);
      for (int k = 0; k < 3; k++) {
        across[k] -= 0.7f * t * sight[k];
      }
      normalize3(across);
      for (int k = 0; k < 3; k++) {
        node[j][k] = node[j - 1][k] + across[k] * seg;
      }
    }
    for (int j = 0; j < nodes; j++) {
      // Thick in the middle, thin at the ends.
      float mid = 1.0f - fabsf(2.0f * static_cast<float>(j) / static_cast<float>(nodes - 1) - 1.0f);
      radius[j] = p.cloudWidth * rng.range(0.7f, 1.3f) * (0.5f + 0.7f * mid);
      if (nodes == 2) {
        radius[j] = p.cloudWidth * rng.range(1.0f, 1.8f);
      }
    }
    // Never over the camera: a segment too near the viewer drops the whole
    // cloud rather than half of it.
    int j = 0;
    for (; j < nodes - 1; j++) {
      if (segmentDistance(viewer, node[j], node[j + 1]) <
          2.5f * fmaxf(radius[j], radius[j + 1]) + 0.03f) {
        break;
      }
    }
    if (j < nodes - 1) {
      continue;
    }
    for (j = 0; j < nodes - 1; j++) {
      Pillar& pl = s.pillar[s.pillarCount++];
      std::memcpy(pl.base, node[j], sizeof(pl.base));
      std::memcpy(pl.tip, node[j + 1], sizeof(pl.tip));
      pl.baseRadius = radius[j];
      pl.tipRadius = radius[j + 1];
      pl.adrift = true;
      b.pillarCount++;
    }
  }
}

void shapeBubble(Random& rng, Bubble& b, const SceneParams& p) {
  randomRotation(rng, b.rot);
  b.thickness = p.thickness * rng.range(0.8f, 1.25f);
  b.fold = p.fold * rng.range(0.8f, 1.25f);
  b.keep = p.keep;
  // Anywhere in the tiling volume; a period's worth of range is all there is.
  b.noiseOffset = rng.range(0.0f, 64.0f);
  b.squeeze[0] = b.squeeze[1] = b.squeeze[2] = 1.0f;
  b.edge = 1.0f;
  b.density = 1.0f;
}

// A distant bubble's form. Real nebulae seen from outside are lobes, arcs and
// irregular shells, never spheres: squeezed along two axes by up to about two,
// one kept whole so it keeps its angular size; and a soft outer edge, since
// from outside the edge is the rim, and a hard rim is what draws the circle.
void shapeDistant(Random& rng, Bubble& b) {
  int keepAxis = rng.below(3);
  for (int k = 0; k < 3; k++) {
    b.squeeze[k] = k == keepAxis ? 1.0f : rng.range(1.0f, 2.2f);
  }
  b.edge = rng.range(0.15f, 0.35f);
  // Thicker and less folded. A thin, deeply folded shell seen from inside is
  // filaments; from outside every fold is a crease seen edge on, and a few
  // degrees of them read as crumpled paper.
  float thicker = rng.range(2.0f, 3.0f);
  b.thickness *= thicker;
  b.fold *= 0.6f;
  // At the same column, or its dust blocks the glow behind it and the nebula
  // sits in a black silhouette of itself.
  b.density = 1.0f / thicker;
}

}  // namespace

float bubbleBound(const Bubble& b) {
  // The fold warp is two octaves of noise of about 0.7 and 0.35 at most, and
  // the thickness varies up to 1.4 times nominal with a gaussian profile gone
  // to nothing by three of them.
  return 1.0f + 1.2f * b.fold + 4.2f * b.thickness;
}

void bubbleToSky(const Bubble& b, const float local[3], float out[3]) {
  for (int i = 0; i < 3; i++) {
    out[i] = b.center[i] +
             b.radius * (b.rot[0 + i] * local[0] + b.rot[3 + i] * local[1] + b.rot[6 + i] * local[2]);
  }
}

Scene generateScene(const SceneParams& p) {
  Scene s{};
  Random rng(p.seed * 2654435761u + 17u);
  Bubble& main = s.bubble[0];
  float dir[3];

  s.seed = p.seed;
  rng.unitVector(dir);
  main.radius = 1.0f;
  for (int k = 0; k < 3; k++) {
    main.center[k] = -dir[k] * p.viewerOffset;
  }
  shapeBubble(rng, main, p);
  int n = p.clusters < 1 ? 1 : (p.clusters > kMaxClusters ? kMaxClusters : p.clusters);
  lightBubble(rng, main, n, p.clusterOffset, p.luminosity);
  raisePillars(p.seed, 0, s, main, p.pillars, p);
  // Straight after the main bubble's pillars, so they share its range.
  setCloudsAdrift(p.seed, s, main, p);
  s.bubbleCount = 1;

  float mainReach = p.viewerOffset + bubbleBound(main);
  n = p.distantCount > kMaxBubbles - 1 ? kMaxBubbles - 1 : p.distantCount;
  for (int i = 0; i < n; i++) {
    Bubble& b = s.bubble[s.bubbleCount];
    float lo = p.distantMinDegrees, hi = p.distantMaxDegrees;

    shapeBubble(rng, b, p);
    shapeDistant(rng, b);
    // Skewed toward the small end: many modest nebulae and the odd big one
    // reads as depth, where equal sizes read as a pattern.
    float degrees = lo + (hi - lo) * powf(rng.uniform(), 2.0f);
    float alpha = degrees * kPi / 180.0f;
    b.radius = rng.range(0.5f, 1.5f);
    float reach = b.radius * bubbleBound(b);
    float distance = b.radius / sinf(alpha);
    // Wholly beyond the main bubble, or the two would be marched as if one
    // were in front of the other when they are intermingled. Scaling radius
    // and distance together keeps the angle; an angle so wide the bound would
    // reach back past the viewer cannot be kept at any scale, and is given up
    // for simply standing it off.
    if (distance - reach < mainReach) {
      if (distance > reach) {
        float scale = 1.05f * mainReach / (distance - reach);
        b.radius *= scale;
        reach *= scale;
        distance *= scale;
      } else {
        distance = 1.05f * (mainReach + reach);
      }
    }
    rng.unitVector(dir);
    for (int k = 0; k < 3; k++) {
      b.center[k] = dir[k] * distance;
    }
    // Clusters inside the squeezed shell, and dimmer than the main bubble's: a
    // nebula seen across the disc is behind a kiloparsec or so of dust, a
    // magnitude or more; at full brightness it stands out like a decal.
    //
    // Drawn in this order -- brightness, offset, count -- because the labs drew
    // all three in one call's arguments, which GCC evaluates right to left, and
    // seeds are kept compatible with the skies made there.
    float brightness = p.luminosity * rng.range(0.2f, 0.6f);
    float offset = rng.range(0.1f, 0.5f) / fmaxf(b.squeeze[0], fmaxf(b.squeeze[1], b.squeeze[2]));
    int count = 1 + rng.below(2);
    lightBubble(rng, b, count, offset, brightness);
    // No pillars: at a few degrees a pillar's dark core is a black blot.
    raisePillars(p.seed, s.bubbleCount, s, b, 0, p);
    s.bubbleCount++;
  }
  return s;
}

}  // namespace starcanopy
