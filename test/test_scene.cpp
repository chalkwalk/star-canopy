// A seed makes the same scene as in the labs, float for float, where look 2
// kept it: every bubble's place, turn, stride, fold, holes, noise and squeeze,
// and the main bubble's clusters and their stars. The expected hashes are over
// those fields, with the labs' two clusters; they were computed by look 1,
// whose scenes the labs' own nebula_sky_model.c had matched in full, and look
// 2 matching them is what says the shell's retirement moved no draw of the
// seed's. (Look 2 moves the distant bubbles' clusters out and lights them from
// beside; see the tests below.)
//
// Float for float holds where the maths library is glibc's, as the labs' was.
// Another library's sin or pow may differ in a last bit, which moves no draw
// and no pixel (test_look agrees) but would move an exact hash; so everywhere
// the fields are also hashed rounded to 12 bits of mantissa, about 1 part in
// 4000 -- far finer than any change to what a seed draws.

#include "check.h"
#include "scene.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace starcanopy;

namespace {

uint32_t h;
bool coarse;

void f(float v) {
  uint32_t u;
  std::memcpy(&u, &v, 4);
  if (coarse) {
    u = (u + 0x400u) & ~0x7ffu;
  }
  for (int i = 0; i < 4; i++) {
    h ^= (u >> (8 * i)) & 255u;
    h *= 16777619u;
  }
}

void fv(const float* v, int n) {
  for (int i = 0; i < n; i++) {
    f(v[i]);
  }
}

uint32_t sceneHash(const Scene& s, bool rounded = false) {
  coarse = rounded;
  h = 2166136261u;
  f(static_cast<float>(s.bubbleCount));
  for (int b = 0; b < s.bubbleCount; b++) {
    const Bubble& u = s.bubble[b];
    fv(u.center, 3);
    f(u.radius);
    fv(u.rot, 9);
    f(u.stride);
    f(u.fold);
    f(u.keep);
    f(u.noiseOffset);
    fv(u.squeeze, 3);
    f(u.density);
    if (b == 0) {
      for (const Cluster& c : u.cluster) {
        fv(c.pos, 3);
        f(c.luminosity);
        for (const ClusterStar& st : c.star) {
          fv(st.dir, 3);
          f(st.intensity);
        }
      }
    }
  }
  return h;
}

float length3(const float v[3]) {
  return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

}  // namespace

int main() {
  const struct {
    uint32_t seed;
    int bubbles;
    uint32_t hash;
    uint32_t rounded;
  } expected[] = {
    {1, 4, 4256231499u, 2316976348u},
    {7, 4, 847006508u, 2652156954u},
    {12345, 4, 3017844379u, 3705940563u},
  };
  for (const auto& e : expected) {
    SceneParams p;
    p.seed = e.seed;
    p.clusters = 2;
    Scene s = generateScene(p);
    uint32_t got = sceneHash(s, false);
    uint32_t rounded = sceneHash(s, true);
    if (rounded != e.rounded) {
      std::printf("seed %u: rounded hash %u, want %u\n", e.seed, rounded, e.rounded);
    }
    CHECK(s.bubbleCount == e.bubbles);
    CHECK(rounded == e.rounded);
#ifdef __GLIBC__
    if (got != e.hash) {
      std::printf("seed %u: hash %u, want %u\n", e.seed, got, e.hash);
    }
    CHECK(got == e.hash);
#endif
  }

  // A distant bubble is lit from beside it: its clusters outside it, 1.4 of
  // its radii from its centre, and mostly across our line of sight to it.
  for (uint32_t seed = 1; seed <= 20; seed++) {
    SceneParams p;
    p.seed = seed;
    Scene s = generateScene(p);
    for (int b = 1; b < s.bubbleCount; b++) {
      const Bubble& u = s.bubble[b];
      float toViewer[3];
      for (int k = 0; k < 3; k++) {
        toViewer[k] = -u.center[k];
      }
      float d = length3(toViewer);
      for (const Cluster& c : u.cluster) {
        if (c.luminosity <= 0.0f) {
          continue;
        }
        CHECK(std::fabs(length3(c.pos) - 1.4f) < 1e-3f);
        // The cluster in sky units, from the bubble's centre, against the
        // line to the viewer: well off it either way.
        float sky[3], rel[3];
        bubbleToSky(u, c.pos, sky);
        for (int k = 0; k < 3; k++) {
          rel[k] = sky[k] - u.center[k];
        }
        float cosine = (rel[0] * toViewer[0] + rel[1] * toViewer[1] + rel[2] * toViewer[2]) /
                       (length3(rel) * d);
        CHECK(std::fabs(cosine) < 0.9f);
      }
    }
  }

  // The same params, the same scene; a different seed, a different one.
  SceneParams p;
  p.seed = 99;
  CHECK(sceneHash(generateScene(p)) == sceneHash(generateScene(p)));
  uint32_t a = sceneHash(generateScene(p));
  p.seed = 100;
  CHECK(sceneHash(generateScene(p)) != a);

  // Every distant bubble lies wholly beyond the main one, so ordering them by
  // where a ray enters is ordering them front to back.
  for (uint32_t seed = 1; seed <= 50; seed++) {
    p.seed = seed;
    Scene s = generateScene(p);
    float mainReach = p.viewerOffset + bubbleBound(s.bubble[0]);
    for (int b = 1; b < s.bubbleCount; b++) {
      const Bubble& u = s.bubble[b];
      float d = std::sqrt(u.center[0] * u.center[0] + u.center[1] * u.center[1] +
                          u.center[2] * u.center[2]);
      CHECK(d - u.radius * bubbleBound(u) >= mainReach * 0.999f);
    }
  }
  return test::finish();
}
