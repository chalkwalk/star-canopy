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

void shapeBubble(Random& rng, Bubble& b, const SceneParams& p) {
  randomRotation(rng, b.rot);
  b.stride = p.stride * rng.range(0.8f, 1.25f);
  b.fold = p.fold * rng.range(0.8f, 1.25f);
  b.keep = p.keep;
  // Anywhere in the tiling volume; a period's worth of range is all there is.
  b.noiseOffset = rng.range(0.0f, 64.0f);
  b.squeeze[0] = b.squeeze[1] = b.squeeze[2] = 1.0f;
  b.density = 1.0f;
  b.veil[0] = b.veil[1] = b.veil[2] = 1.0f;
}

// A distant bubble's form: squeezed along two axes by up to about two, one
// kept whole so it keeps its angular size, so it is a lobe or an arc and not a
// sphere. Its stride is longer and its gas thinner in step, as look 1's
// shells were thicker: kept, so the distant nebulae judged blind are these.
void shapeDistant(Random& rng, Bubble& b) {
  int keepAxis = rng.below(3);
  for (int k = 0; k < 3; k++) {
    b.squeeze[k] = k == keepAxis ? 1.0f : rng.range(1.0f, 2.2f);
  }
  // Look 1's shells drew their edge's softness here; drawn still, and
  // dropped, so the rest of the stream is as it was.
  rng.range(0.15f, 0.35f);
  float longer = rng.range(2.0f, 3.0f);
  b.stride *= longer;
  // Less folded: from outside every fold is a crease seen edge on.
  b.fold *= 0.6f;
  b.density = 1.0f / longer;
}

}  // namespace

float bubbleBound(const Bubble& b) {
  // The fold warp is two octaves of noise of about 0.7 and 0.35 at most; and
  // four strides, which were look 1's shell's reach, kept so that the bound --
  // and so the light volume's box, and the sky -- are as they were.
  return 1.0f + 1.2f * b.fold + 4.2f * b.stride;
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
    // Clusters inside the squeezed bubble, and dimmer than the main bubble's: a
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
    // A distant mass is lit from beside it: seen from outside, one lit from
    // within glows evenly all through, and nothing says which way the light
    // falls, so it has no form (distant, blind, round 3). So its clusters go
    // outside it, and round toward the side as we see it -- lit from behind
    // it would be a black silhouette, from in front a flat disc. After the
    // draws, which stay as they were.
    static const float origin[3] = {0.0f, 0.0f, 0.0f};
    float view[3];
    skyToBubble(b, origin, view);
    float vl = sqrtf(dot3(view, view));
    for (int k = 0; k < 3; k++) {
      view[k] /= vl;
    }
    for (int c = 0; c < count; c++) {
      float* pos = b.cluster[c].pos;
      // Brighter for being further: by the distance, not its square, which
      // keeps its heart lit as from within but made them too bright beside
      // the main nebula (distant, blind, round 4).
      float was = fmaxf(sqrtf(dot3(pos, pos)), 0.2f);
      b.cluster[c].luminosity *= 1.4f / was;
      float along = dot3(pos, view);
      for (int k = 0; k < 3; k++) {
        pos[k] -= 0.8f * along * view[k];
      }
      float len = fmaxf(sqrtf(dot3(pos, pos)), 1e-4f);
      for (int k = 0; k < 3; k++) {
        pos[k] *= 1.4f / len;
      }
    }
    // Their stars again where the clusters now are, from a stream of their
    // own so the scene's stays as it was.
    Random stars(p.seed * 2246822519u + 97u * static_cast<uint32_t>(s.bubbleCount));
    for (int c = 0; c < count; c++) {
      placeStars(stars, b, b.cluster[c]);
    }
    s.bubbleCount++;
  }
  return s;
}

}  // namespace starcanopy
