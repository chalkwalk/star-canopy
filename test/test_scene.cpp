// A seed makes the same scene as in the labs, float for float. The expected
// hashes were computed by the labs' own nebula_sky_model.c over the same
// fields in the same order, with the labs' default dials.

#include "check.h"
#include "scene.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace starcanopy;

namespace {

uint32_t h;

void f(float v) {
  uint32_t u;
  std::memcpy(&u, &v, 4);
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

uint32_t sceneHash(const Scene& s) {
  h = 2166136261u;
  f(static_cast<float>(s.bubbleCount));
  for (int b = 0; b < s.bubbleCount; b++) {
    const Bubble& u = s.bubble[b];
    fv(u.center, 3);
    f(u.radius);
    fv(u.rot, 9);
    f(u.thickness);
    f(u.fold);
    f(u.keep);
    f(u.noiseOffset);
    fv(u.squeeze, 3);
    f(u.edge);
    f(u.density);
    for (const Cluster& c : u.cluster) {
      fv(c.pos, 3);
      f(c.luminosity);
      for (const ClusterStar& st : c.star) {
        fv(st.dir, 3);
        f(st.intensity);
      }
    }
    f(static_cast<float>(u.firstPillar));
    f(static_cast<float>(u.pillarCount));
  }
  f(static_cast<float>(s.pillarCount));
  for (int i = 0; i < s.pillarCount; i++) {
    const Pillar& p = s.pillar[i];
    fv(p.base, 3);
    f(p.baseRadius);
    fv(p.tip, 3);
    f(p.tipRadius);
    f(p.adrift ? 1.0f : 0.0f);
  }
  return h;
}

}  // namespace

int main() {
  const struct {
    uint32_t seed;
    int bubbles, pillars;
    uint32_t hash;
  } expected[] = {
    {1, 4, 20, 3886375356u},
    {7, 4, 23, 3116510298u},
    {12345, 4, 20, 1847972669u},
  };
  for (const auto& e : expected) {
    SceneParams p;
    p.seed = e.seed;
    Scene s = generateScene(p);
    uint32_t got = sceneHash(s);
    if (got != e.hash) {
      std::printf("seed %u: hash %u, want %u\n", e.seed, got, e.hash);
    }
    CHECK(s.bubbleCount == e.bubbles);
    CHECK(s.pillarCount == e.pillars);
    CHECK(got == e.hash);
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
