# Open Clusters Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Open clusters in the sky as accents -- irregular, age-coloured knots of real stars placed where the galaxy's young stars are -- on a placer every later accent kind reuses.

**Architecture:** A new unit, `accents.h/.cpp`, draws accents from its own random stream, galaxy-weighted with a near bias. `stars.cpp` turns each open cluster into member stars (the drawable ones) and records the light of the rest; `galaxy.shader` adds that light as a faint haze where each ray's march passes the cluster's distance, behind exactly the dust in front. Default off (`open-clusters = 0`) until a blind round makes it look 10.

**Tech Stack:** C++17, CMake/Ninja, OpenGL 3.3 core, GLSL 1.50, the project's own `check.h` tests under ctest.

**Spec:** `docs/superpowers/specs/2026-09-29-open-clusters-design.md`

## Global Constraints

- C++17, 2-space indent, braces on the same line, lowerCamelCase members, `#pragma once`, namespace `starcanopy`; comments say why, not what (`AGENTS.md`).
- Shaders stay ASCII GLSL 1.50.
- Stage files by name; never `git add -A`, `git add .` or `git commit -a`. Commit messages: a one-line imperative summary of the effect, then the why, ending `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Do not push.
- No reference imagery in the tree; the game the project serves is named only in `README.md`.
- A change to any sky's pixels is a new look version, made only after a blind comparison scored by the user (`PRINCIPLES §1`, `§2`); `test_look` is never re-recorded otherwise. Tasks 1-3 must leave `test_look` passing: `open-clusters` defaults to 0 until Task 5.
- Never enlarge an accent beyond what its distance allows (`PRINCIPLES §4`); open clusters are never round, never a glowing ball, never uniformly gold.
- Before claiming a task works: `cmake --build build` clean, `ctest --test-dir build --output-on-failure` green, output quoted.

## Review Focus

- `band-stars = 0` (no flux limit): every member is drawn as a star, no remainder, no haze, no division by zero.
- An observer far from the young disc -- galactic -1, past the edge, the atlas's portrait: few or no clusters, the placement loop terminating promptly, never clusters conjured in the empty halo.
- A lenticular galaxy, whose arms (and so young stars) are nearly nil: still terminates, still a few clusters from the old disc's small share.
- More than 16 open clusters (`open-clusters = 40`): all their members drawn, the haze only for the 16 brightest, no shader array overrun.
- A cluster very near or around the observer (`accent-near = 1`, an association 50 pc off with a 60 pc radius): finite fluxes, members no nearer than `kMinDistance`, no NaN.

---

### Task 1: The accents' placer and its dials

**Files:**
- Create: `src/core/accents.h`, `src/core/accents.cpp`
- Create: `test/test_accents.cpp`
- Modify: `src/core/CMakeLists.txt` (add `accents.cpp` to `starcanopy_core`)
- Modify: `test/CMakeLists.txt` (add `accents` to `tests`, and `add_test(NAME accents COMMAND test_accents)`)
- Modify: `src/core/settings.h` (fields; `Sky::accents`), `src/core/settings.cpp` (dials; `buildSky`)

**Interfaces:**
- Consumes: `Galaxy`, `galaxyDensity(const Galaxy&, const float p[3], float footprint)`, `GalaxySample{old, young, dust}` (`galaxy.h`); `Random` (`random.h`).
- Produces:
  ```cpp
  enum AccentKind { kOpenCluster };
  constexpr int kMaxAccentGlows = 16;
  struct AccentParams { int openClusters = 0; float near = 0.4f; };
  struct Accent {
    int kind;
    bool association;   // the loose end: large, few, young
    float pos[3];       // kpc, galaxy frame
    float offset[3];    // kpc, galaxy frame, from the observer
    float dir[3];       // unit, sky frame
    float distance;     // kpc
    float radius;       // kpc: how far its lumps spread
    uint32_t seed;      // for its details
    float glow[3];      // filled by generateStars(): members too faint to draw
  };
  std::vector<Accent> generateAccents(uint32_t seed, const Galaxy& g, const AccentParams& p);
  // Settings: int openClusters = 0; float accentNear = 0.4f;  Sky: AccentParams accents;
  ```

- [ ] **Step 1: Write the failing test** -- `test/test_accents.cpp`:

```cpp
// The accents: open clusters placed where the galaxy's young stars are, as many
// as the dial asks about a place in the band, nearer as accent-near rises, the
// far ones in the disc; none where there is no disc to be born in.

#include "accents.h"
#include "check.h"
#include "galaxy.h"
#include "settings.h"

#include <cmath>
#include <cstdio>

using namespace starcanopy;

namespace {

Galaxy galaxyAt(uint32_t seed, float radius, float height) {
  GalaxyParams gp;
  gp.observerRadius = radius;
  gp.observerHeight = height;
  return generateGalaxy(seed, gp);
}

}  // namespace

int main() {
  AccentParams p;
  p.openClusters = 6;
  Galaxy g = galaxyAt(1, 2.6f, 0.0f);

  // One seed, one set; none when off.
  std::vector<Accent> a = generateAccents(1, g, p), b = generateAccents(1, g, p);
  CHECK(a.size() == b.size());
  for (size_t i = 0; i < a.size() && i < b.size(); i++) {
    CHECK(a[i].distance == b[i].distance && a[i].seed == b[i].seed);
  }
  AccentParams off = p;
  off.openClusters = 0;
  CHECK(generateAccents(1, g, off).empty());

  // About the dial's count in the band, over many seeds.
  double total = 0.0, nearTotal = 0.0, farTotal = 0.0;
  int nearCount = 0, farCount = 0, far = 0, farInDisc = 0;
  AccentParams close = p, physical = p;
  close.near = 1.0f;
  physical.near = 0.0f;
  for (uint32_t seed = 1; seed <= 40; seed++) {
    Galaxy gs = galaxyAt(seed, 2.6f, 0.0f);
    std::vector<Accent> list = generateAccents(seed, gs, p);
    total += list.size();
    for (const Accent& x : list) {
      CHECK(x.kind == kOpenCluster);
      CHECK(std::isfinite(x.distance) && x.distance > 0.0f && x.radius > 0.0f);
      CHECK(std::fabs(std::sqrt(x.dir[0] * x.dir[0] + x.dir[1] * x.dir[1] + x.dir[2] * x.dir[2]) -
                      1.0f) < 1e-4f);
      if (x.distance > 1.0f) {
        far++;
        farInDisc += std::fabs(x.pos[2]) < 0.5f;
      }
    }
    for (const Accent& x : generateAccents(seed, gs, close)) {
      nearTotal += x.distance;
      nearCount++;
    }
    for (const Accent& x : generateAccents(seed, gs, physical)) {
      farTotal += x.distance;
      farCount++;
    }
  }
  double mean = total / 40.0;
  std::printf("in the band: %.2f a sky; mean distance near %.2f kpc, physical %.2f kpc; "
              "%d of %d far ones in the disc\n",
              mean, nearTotal / nearCount, farTotal / farCount, farInDisc, far);
  CHECK(mean > 6.0 * 0.6 && mean < 6.0 * 1.6);
  CHECK(nearCount > 0 && farCount > 0 && nearTotal / nearCount < 0.6 * (farTotal / farCount));
  CHECK(far > 0 && farInDisc >= 0.9 * far);

  // Far out, few: the young disc is far away, and none are made in the halo.
  double remote = 0.0;
  for (uint32_t seed = 1; seed <= 20; seed++) {
    remote += generateAccents(seed, galaxyAt(seed, 7.0f, 1.5f), p).size();
  }
  std::printf("far out: %.2f a sky\n", remote / 20.0);
  CHECK(remote / 20.0 < 0.5 * mean);

  // A lenticular, with next to no young stars, still finishes and has a few.
  GalaxyParams lp;
  lp.observerRadius = 2.6f;
  lp.observerHeight = 0.0f;
  lp.style = kLenticular;
  std::vector<Accent> l = generateAccents(3, generateGalaxy(3, lp), p);
  std::printf("a lenticular: %zu\n", l.size());
  CHECK(l.size() <= 40);

  // The dials reach the sky.
  Settings s;
  CHECK(s.openClusters == 0);
  s.openClusters = 9;
  s.accentNear = 0.7f;
  Sky sky = buildSky(s);
  CHECK(sky.accents.openClusters == 9 && sky.accents.near == 0.7f);
  return test::finish();
}
```

- [ ] **Step 2: Register the test and run it to see it fail**

In `test/CMakeLists.txt`, add `accents` to the `set(tests ...)` line and `add_test(NAME accents COMMAND test_accents)` after `add_test(NAME stars ...)`.

Run: `cmake --build build 2>&1 | tail -3`
Expected: FAIL to compile -- `accents.h: No such file or directory`.

- [ ] **Step 3: Write `src/core/accents.h`**

```cpp
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
```

- [ ] **Step 4: Write `src/core/accents.cpp`**

```cpp
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
// own, so k = 2 is physical and lower tilts the draw toward the near.
void propose(Random& rng, float k, float offset[3], float& d) {
  rng.unitVector(offset);
  float e = k + 1.0f;
  float a = powf(kNearest, e), b = powf(kFarthest, e);
  d = powf(a + rng.uniform() * (b - a), 1.0f / e);
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
  float k = 2.0f - 2.0f * fminf(fmaxf(p.near, 0.0f), 1.0f);

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
    a.association = rng.uniform() < 0.15f;
    a.radius = a.association ? rng.range(0.02f, 0.06f) : rng.range(0.003f, 0.012f);
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
```

In `src/core/CMakeLists.txt`, add `accents.cpp` as the first entry of `add_library(starcanopy_core STATIC ...)`.

- [ ] **Step 5: The dials and the sky**

`src/core/settings.h`: add `#include "accents.h"` beside the other includes; after `int externalGalaxies = 4;` add

```cpp
  int openClusters = 0;     // open clusters about a sky; 0 none (look 10 sets it)
  float accentNear = 0.4f;  // how strongly accents are drawn near, 0..1
```

and in `struct Sky`, after `StarParams stars;`, add `AccentParams accents;`.

`src/core/settings.cpp`: after the `INT("external-galaxies", ...)` dial, add

```cpp
  INT("open-clusters", "open clusters about a sky, typically -- knots of young stars; 0 none",
      openClusters, 0.0f, 40.0f),
  REAL("accent-near", "how strongly the sky's objects are drawn near: 0 as physics has them, "
       "1 mostly near", accentNear, 0.0f, 1.0f),
```

and in `buildSky()`, after the `StarParams& st = sky.stars;` block, add

```cpp
  sky.accents.openClusters = s.openClusters;
  sky.accents.near = s.accentNear;
```

- [ ] **Step 6: Run the tests**

Run: `cmake --build build -j $(nproc) 2>&1 | grep -E "error|warning"; ./build/test/test_accents`
Expected: no build output; the three printed lines, then no `check failed`. If `mean` is outside 3.6-9.6, the reference place is wrong; if the far-out count is not under half, `reach()` is not scaling -- fix, do not loosen the check.

Run: `ctest --test-dir build --output-on-failure 2>&1 | tail -4`
Expected: `100% tests passed` (the dials default off, so `test_look` is unmoved).

- [ ] **Step 7: Commit**

```bash
git add src/core/accents.h src/core/accents.cpp src/core/CMakeLists.txt src/core/settings.h src/core/settings.cpp test/test_accents.cpp test/CMakeLists.txt
git commit -F - <<'EOF'
Place open clusters where the galaxy's young stars are

The accents' placer, for every kind to come: galaxy-weighted, at physical
distances tilted toward the near by accent-near, as many as open-clusters asks
about a place in the band and fewer where there is less young disc in reach.
Off by default until a blind round.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 2: Members, as stars

**Files:**
- Modify: `src/core/stars.h` (signature), `src/core/stars.cpp` (new `addOpenClusterStars`, called from `generateStars`)
- Modify: `test/test_accents.cpp` (member checks)

**Interfaces:**
- Consumes: `Accent`, `generateAccents` (Task 1); in `stars.cpp`: `kLumIndex`, `kMaxLum`, `kMaxFlux`, `kMinDistance`, `blackbody`, `dustDepth`, `normalize3`.
- Produces:
  ```cpp
  std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p,
                                  float* bandFlux = nullptr,
                                  std::vector<Accent>* accents = nullptr);
  // Each open-cluster accent's glow[3] is set: the light of its members too
  // faint to draw, before the dust in front.
  ```

- [ ] **Step 1: Write the failing tests** -- append to `test/test_accents.cpp`, before `return test::finish();`, adding `#include "scene.h"` and `#include "stars.h"` at the top:

```cpp
  // Members: a cluster turned into stars, spread to its own angular size and
  // never round; behind dust, dimmer; with no flux limit, every one drawn.
  {
    Settings cs;
    cs.seed = 5;
    Sky csky = buildSky(cs);
    Scene scene = generateScene(csky.scene);
    Galaxy cg = generateGalaxy(5, csky.galaxy);
    StarParams sp = csky.stars;
    sp.count = 0;      // no field stars, for a clean count
    sp.bandCount = 0;  // no band: no limit, every member drawn
    Accent one{};
    one.kind = kOpenCluster;
    one.distance = 0.3f;
    one.radius = 0.008f;
    one.seed = 12345u;
    float toward[3] = {1.0f, 0.0f, 0.0f};  // galaxy frame, along the plane
    for (int j = 0; j < 3; j++) {
      one.offset[j] = toward[j] * one.distance;
      one.pos[j] = cg.observer[j] + one.offset[j];
      one.dir[j] = (cg.rot[0 + j] * one.offset[0] + cg.rot[3 + j] * one.offset[1] +
                    cg.rot[6 + j] * one.offset[2]) / one.distance;
    }
    std::vector<Accent> list = {one};
    size_t before = generateStars(scene, cg, sp).size();
    std::vector<Star> all = generateStars(scene, cg, sp, nullptr, &list);
    size_t members = all.size() - before;
    std::printf("a cluster 300 pc off: %zu members drawn\n", members);
    CHECK(members >= 200);
    CHECK(list[0].glow[0] == 0.0f && list[0].glow[1] == 0.0f && list[0].glow[2] == 0.0f);

    // Its spread, about its own direction, against radius over distance; and
    // its shape, from the spread's two axes across the line of sight.
    double sxx = 0.0, syy = 0.0, sxy = 0.0;
    float u[3], v[3];
    const float* w = list[0].dir;
    float helper[3] = {std::fabs(w[0]) < 0.9f ? 1.0f : 0.0f, std::fabs(w[0]) < 0.9f ? 0.0f : 1.0f, 0.0f};
    u[0] = helper[1] * w[2] - helper[2] * w[1];
    u[1] = helper[2] * w[0] - helper[0] * w[2];
    u[2] = helper[0] * w[1] - helper[1] * w[0];
    float un = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    for (float& c : u) {
      c /= un;
    }
    v[0] = w[1] * u[2] - w[2] * u[1];
    v[1] = w[2] * u[0] - w[0] * u[2];
    v[2] = w[0] * u[1] - w[1] * u[0];
    bool finite = true;
    for (size_t i = before; i < all.size(); i++) {
      const Star& st = all[i];
      double x = st.dir[0] * u[0] + st.dir[1] * u[1] + st.dir[2] * u[2];
      double y = st.dir[0] * v[0] + st.dir[1] * v[1] + st.dir[2] * v[2];
      sxx += x * x;
      syy += y * y;
      sxy += x * y;
      finite = finite && std::isfinite(st.flux[0]) && std::isfinite(st.flux[1]) &&
               std::isfinite(st.flux[2]) && st.distance > 0.0f;
    }
    CHECK(finite);
    sxx /= members;
    syy /= members;
    sxy /= members;
    double spread = std::sqrt(sxx + syy), expected = one.radius / one.distance;
    double tr = sxx + syy, det = sxx * syy - sxy * sxy;
    double l1 = tr / 2 + std::sqrt(std::fmax(tr * tr / 4 - det, 0.0));
    double l2 = tr / 2 - std::sqrt(std::fmax(tr * tr / 4 - det, 0.0));
    double elongation = std::sqrt(l1 / std::fmax(l2, 1e-30));
    std::printf("spread %.4f rad against %.4f; elongated %.2f to 1\n", spread, expected, elongation);
    CHECK(spread > 0.5 * expected && spread < 3.0 * expected);
    CHECK(elongation > 1.25);  // never round (globulars are)

    // Behind the galaxy's dust, dimmer: the same cluster, the dust turned off.
    GalaxyParams clear = csky.galaxy;
    clear.dust = 0.0f;
    Galaxy clean = generateGalaxy(5, clear);
    std::vector<Accent> list2 = {one};
    std::vector<Star> dusty = generateStars(scene, cg, sp, nullptr, &list), bare =
        generateStars(scene, clean, sp, nullptr, &list2);
    double fd = 0.0, fb = 0.0;
    for (size_t i = before; i < dusty.size(); i++) {
      fd += dusty[i].flux[1];
    }
    for (size_t i = generateStars(scene, clean, sp).size(); i < bare.size(); i++) {
      fb += bare[i].flux[1];
    }
    CHECK(fd < fb);

    // With a limit, the faint ones become its glow, and none is lost: at 3 kpc
    // most members are too faint.
    // (The field stars stay: without them the band sets no limit.)
    StarParams limited = csky.stars;
    std::vector<Accent> far = {one};
    for (int j = 0; j < 3; j++) {
      far[0].offset[j] = toward[j] * 3.0f;
      far[0].pos[j] = cg.observer[j] + far[0].offset[j];
    }
    far[0].distance = 3.0f;
    float limit = 0.0f;
    generateStars(scene, cg, limited, &limit, &far);
    CHECK(limit > 0.0f && far[0].glow[1] > 0.0f);

    // Around the observer: an association 50 pc off, 60 pc across.
    std::vector<Accent> around = {one};
    around[0].association = true;
    around[0].radius = 0.06f;
    for (int j = 0; j < 3; j++) {
      around[0].offset[j] = toward[j] * 0.05f;
      around[0].pos[j] = cg.observer[j] + around[0].offset[j];
    }
    around[0].distance = 0.05f;
    std::vector<Star> near = generateStars(scene, cg, sp, nullptr, &around);
    bool ok = true;
    for (size_t i = before; i < near.size(); i++) {
      ok = ok && std::isfinite(near[i].flux[1]) && near[i].distance > 0.0f;
    }
    CHECK(ok);
  }
```

- [ ] **Step 2: Run to see it fail**

Run: `cmake --build build 2>&1 | grep error | head -3`
Expected: FAIL -- no matching `generateStars` with five arguments.

- [ ] **Step 3: The signature** -- in `src/core/stars.h`, add `#include "accents.h"`, and replace the declaration with

```cpp
// Deterministic in the scene's seed and the params. bandFlux, if given, is set
// to the flux below which the band's stars are left to the galaxy's glow.
// accents, if given, have their open clusters' members added as stars -- those
// bright enough to draw, as the band's are -- and each one's glow set to the
// light of the rest.
std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p,
                                float* bandFlux = nullptr,
                                std::vector<Accent>* accents = nullptr);
```

- [ ] **Step 4: The members** -- in `src/core/stars.cpp`, inside the anonymous namespace after `addBandStars()`:

```cpp
// An open cluster's members (docs/superpowers/specs/2026-09-29-open-clusters-design.md):
// a few overlapping lumps stretched along one axis, never round, since a round
// glowing ball is a globular's; an age that has taken its most luminous, blue
// stars and left a few red giants; each dimmed by the dust on its own line.
// Those fainter than the band's limit, seen, are its glow instead; with no
// limit, every one is drawn.
void addOpenClusterStars(std::vector<Star>& stars, const Galaxy& g, const StarParams& p,
                         float limit, std::vector<Accent>& accents) {
  const float pi = 3.14159265f;
  float typical = 0.7f * p.reach, typical2 = typical * typical;
  for (Accent& a : accents) {
    if (a.kind != kOpenCluster) {
      continue;
    }
    Random rng(a.seed);
    int members = a.association ? static_cast<int>(rng.range(30.0f, 120.0f))
                                : static_cast<int>(expf(rng.range(logf(200.0f), logf(3000.0f))));
    float age = a.association ? rng.range(0.0f, 0.2f) : rng.uniform();  // 0 young .. 1 old, log age
    float brightest = powf(kMaxLum, 1.0f - 0.85f * age);
    int giants = static_cast<int>(age * 0.02f * members + 0.5f);
    int lumps = 2 + rng.below(3);
    // Stretched across the line of sight: along it, a stretched cluster would
    // be seen round, and never round is what tells it from a globular.
    float axis[3], centre[4][3], lumpRadius[4], sight[3];
    for (int k = 0; k < 3; k++) {
      sight[k] = a.offset[k] / a.distance;
    }
    rng.perpendicular(sight, axis);
    float stretch = rng.range(1.6f, 2.5f);
    for (int l = 0; l < lumps; l++) {
      rng.unitVector(centre[l]);
      float r = a.radius * 0.6f * cbrtf(rng.uniform());
      for (int k = 0; k < 3; k++) {
        centre[l][k] *= r;
      }
      lumpRadius[l] = a.radius * rng.range(0.35f, 0.6f);
    }
    float rest[3] = {0.0f, 0.0f, 0.0f};
    for (int m = 0; m < members; m++) {
      int l = rng.below(lumps);
      float local[3];
      for (int k = 0; k < 3; k++) {
        float u1 = fmaxf(rng.uniform(), 1e-7f), u2 = rng.uniform();
        local[k] = centre[l][k] + sqrtf(-2.0f * logf(u1)) * cosf(2.0f * pi * u2) * lumpRadius[l];
      }
      float along = local[0] * axis[0] + local[1] * axis[1] + local[2] * axis[2];
      float offset[3];
      for (int k = 0; k < 3; k++) {
        offset[k] = a.offset[k] + local[k] + axis[k] * along * (stretch - 1.0f);
      }
      float d = fmaxf(sqrtf(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]),
                      kMinDistance);
      bool giant = m < giants;
      float lum = giant ? rng.range(30.0f, 300.0f)
                        : fminf(powf(fmaxf(1.0f - rng.uniform(), 1e-6f), -1.0f / kLumIndex), brightest);
      float kelvin = giant ? rng.range(3600.0f, 4600.0f)
                           : fminf(fmaxf(4500.0f * powf(lum, 0.2f), 3000.0f), 30000.0f);
      float rgb[3];
      blackbody(kelvin, rgb);
      float flux = fminf(lum * typical2 / (d * d), kMaxFlux);
      float tau = dustDepth(g, offset);
      if (limit > 0.0f && flux * expf(-tau * p.reddening[1]) < limit) {
        for (int k = 0; k < 3; k++) {
          rest[k] += rgb[k] * flux;
        }
        continue;
      }
      Star st{};
      for (int k = 0; k < 3; k++) {
        st.dir[k] = g.rot[0 + k] * offset[0] + g.rot[3 + k] * offset[1] + g.rot[6 + k] * offset[2];
      }
      normalize3(st.dir);
      st.distance = d / p.kpcPerSkyUnit;
      for (int k = 0; k < 3; k++) {
        st.flux[k] = rgb[k] * flux * expf(-tau * p.reddening[k]);
      }
      stars.push_back(st);
    }
    for (int k = 0; k < 3; k++) {
      a.glow[k] = rest[k];
    }
  }
}
```

and replace `generateStars()`'s definition with

```cpp
std::vector<Star> generateStars(const Scene& s, const Galaxy& g, const StarParams& p,
                                float* bandFlux, std::vector<Accent>* accents) {
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
  // Last, each from its own seed, so every star before is as it was.
  if (accents) {
    addOpenClusterStars(stars, g, p, limit, *accents);
  }
  return stars;
}
```

(`addBandStars` returns 0 when the band is off, which is "no limit".)

- [ ] **Step 5: Run the tests**

Run: `cmake --build build -j $(nproc) 2>&1 | grep -E "error|warning"; ./build/test/test_accents`
Expected: the printed lines; `members drawn` at least 200; `spread` within 0.5-3 of expected; `elongated` above 1.25; no `check failed`.

Run: `ctest --test-dir build --output-on-failure 2>&1 | tail -4`
Expected: `100% tests passed` (nothing passes accents yet outside the test).

- [ ] **Step 6: Commit**

```bash
git add src/core/stars.h src/core/stars.cpp test/test_accents.cpp
git commit -F - <<'EOF'
Make open clusters of real stars

Each open cluster's members: a few lumps stretched along one axis, never round;
an age that has taken the bluest and left a few red giants; each star dimmed by
the dust on its own line. Those too faint to draw are kept as the cluster's
glow, for the galaxy's march; with no band, every member is drawn.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 3: The unresolved haze, and the wiring into a bake

**Files:**
- Modify: `src/core/shaders/galaxy.shader` (accent uniforms; the march adds their light; added after the knee)
- Modify: `src/core/bake.h`, `src/core/bake.cpp` (`kStarFluxUnit` into the header; `bakeGalaxy` takes accents; `readGalaxy()`)
- Modify: `src/core/sky.cpp` (generate accents; pass them to the stars and the glow)
- Modify: `test/test_galaxy.cpp` (the haze is where the cluster is, dimmed by dust, and nowhere else)

**Interfaces:**
- Consumes: `Accent`, `kMaxAccentGlows`, `generateAccents` (Task 1); `generateStars(..., bandFlux, accents)` (Task 2); `Sky::accents`.
- Produces:
  ```cpp
  // bake.h
  constexpr float kStarFluxUnit = 1.0e-6f;
  void Baker::bakeGalaxy(const Galaxy& g, const float reddening[3], int res, const float band[3],
                         float exposure = 1.0f, float knee = 1.5f,
                         const std::vector<Accent>* accents = nullptr, float accentScale = 0.0f);
  Cubemap Baker::readGalaxy() const;  // the glow as last baked, for tests
  ```

- [ ] **Step 1: Write the failing test** -- in `test/test_galaxy.cpp` (which already includes `bake.h`), add `#include "accents.h"`, `#include "cubemap.h"` and `#include <string>`, and before the final `return test::finish();`:

```cpp
  // An open cluster's haze lies where it is, and nowhere else: one cluster
  // straight along the sky's +z, 0.5 kpc off, against none.
  {
    Baker baker;
    std::string why;
    CHECK(baker.ok(why));
    GalaxyParams gp;
    gp.observerRadius = 2.6f;
    gp.observerHeight = 0.0f;
    Galaxy hg = generateGalaxy(1, gp);
    const float reddening[3] = {0.8f, 1.0f, 1.3f}, band[3] = {0.0f, 0.0f, 0.3f};
    Accent c{};
    c.kind = kOpenCluster;
    c.dir[2] = 1.0f;
    c.distance = 0.5f;
    c.radius = 0.01f;
    c.glow[0] = c.glow[1] = c.glow[2] = 1.0e6f;
    std::vector<Accent> one = {c}, none;
    const int res = 32;
    baker.bakeGalaxy(hg, reddening, res, band, 1.0f, 1.5f, &none, 1.0e-6f);
    Cubemap without = baker.readGalaxy();
    baker.bakeGalaxy(hg, reddening, res, band, 1.0f, 1.5f, &one, 1.0e-6f);
    Cubemap with = baker.readGalaxy();
    // Face +z is GL face 4; its middle texel looks along +z; a corner far off.
    auto at = [&](const Cubemap& m, int face, int x, int y) {
      return m.faces[face][(static_cast<size_t>(y) * res + x) * 3 + 1];
    };
    float centre = at(with, 4, res / 2, res / 2) - at(without, 4, res / 2, res / 2);
    float corner = at(with, 4, 0, 0) - at(without, 4, 0, 0);
    float behind = at(with, 5, res / 2, res / 2) - at(without, 5, res / 2, res / 2);
    std::printf("haze: %.4g at the cluster, %.4g off it, %.4g behind\n", centre, corner, behind);
    CHECK(centre > 0.0f);
    CHECK(std::fabs(corner) < 0.01f * centre && std::fabs(behind) < 1e-6f);
  }
```

(`Cubemap` faces are RGB, three floats a texel, as `CubemapTarget::read()` reads them.)

- [ ] **Step 2: Run to see it fail**

Run: `cmake --build build 2>&1 | grep error | head -3`
Expected: FAIL -- `bakeGalaxy` takes no accents; no `readGalaxy`.

- [ ] **Step 3: The shader** -- in `src/core/shaders/galaxy.shader`, after `#define GAL_MAX_EXTERNAL 8` add `#define GAL_MAX_ACCENTS 16`; after the `u_External...` uniforms add

```glsl
/* Accents' unresolved light -- an open cluster's members too faint to draw -- added where the
 * march passes each one's distance, so the dust in front dims it exactly; in star-flux units,
 * turned into this glow's by u_AccentScale.  Added after the exposure and the knee, as the
 * stars are: it is their light. */
uniform int u_AccentCount;
uniform vec4 u_AccentDir[GAL_MAX_ACCENTS];	/* sky direction, core angle (radians) */
uniform vec4 u_AccentGlow[GAL_MAX_ACCENTS];	/* light per channel, distance (kpc) */
uniform float u_AccentScale;
```

In `main()`, declare `vec3 accent = vec3(0.0);` beside `transmit`, and inside the march loop, immediately after `float dt = tb - ta, t = 0.5 * (ta + tb);`:

```glsl
			for (k = 0; k < u_AccentCount; k++) {
				float da = u_AccentGlow[k].w;

				if (da >= ta && da < tb) {
					/* A Plummer core: its light over its whole extent is its glow. */
					float a = u_AccentDir[k].w;
					float th2 = 2.0 * (1.0 - dot(dir, u_AccentDir[k].xyz));
					float q = 1.0 + th2 / (a * a);

					accent += transmit * u_AccentGlow[k].rgb *
						(u_AccentScale / (3.1415927 * a * a * q * q));
				}
			}
```

and replace `glow = bare * dimming;` with `glow = bare * dimming + accent;`.

- [ ] **Step 4: The baker** -- `src/core/bake.h`: add `#include "accents.h"` and, before `class Baker`, `constexpr float kStarFluxUnit = 1.0e-6f;` (with the comment moved from `bake.cpp`), removing the definition from `bake.cpp`. Change the declaration:

```cpp
  // Its light is scaled by exposure and then eased past the knee (0: none).
  // accents' unresolved light, scaled by accentScale into the glow's units,
  // is added after both, as the stars' own light is.
  void bakeGalaxy(const Galaxy& g, const float reddening[3], int res, const float band[3],
                  float exposure = 1.0f, float knee = 1.5f,
                  const std::vector<Accent>* accents = nullptr, float accentScale = 0.0f);

  // The glow as last baked, for tests.
  Cubemap readGalaxy() const;
```

In `bake.cpp`'s `bakeGalaxy`, after the `u_GalKnee` uniform:

```cpp
  // The brightest few accents' haze; the rest are points only.
  float dir[kMaxAccentGlows][4] = {}, glow[kMaxAccentGlows][4] = {};
  int count = 0;
  if (accents) {
    std::vector<const Accent*> order;
    for (const Accent& a : *accents) {
      if (a.glow[0] + a.glow[1] + a.glow[2] > 0.0f) {
        order.push_back(&a);
      }
    }
    std::sort(order.begin(), order.end(), [](const Accent* x, const Accent* y) {
      return x->glow[1] > y->glow[1];
    });
    for (const Accent* a : order) {
      if (count == kMaxAccentGlows) {
        break;
      }
      std::memcpy(dir[count], a->dir, sizeof(a->dir));
      dir[count][3] = fmaxf(0.5f * a->radius / a->distance, 1e-4f);  // the core, half its extent
      std::memcpy(glow[count], a->glow, sizeof(a->glow));
      glow[count][3] = a->distance;
      count++;
    }
  }
  glUniform1i(p.uniform("u_AccentCount"), count);
  glUniform4fv(p.uniform("u_AccentDir"), kMaxAccentGlows, &dir[0][0]);
  glUniform4fv(p.uniform("u_AccentGlow"), kMaxAccentGlows, &glow[0][0]);
  glUniform1f(p.uniform("u_AccentScale"), accentScale);
```

`readGalaxy()`, mirroring `CubemapTarget::read()` for `galaxyTexture_` (same size as the last `bakeGalaxy` res; store it as a member `int galaxyRes_` set in `bakeGalaxy`):

```cpp
Cubemap Baker::readGalaxy() const {
  Cubemap out;
  out.size = galaxyRes_;
  glBindTexture(GL_TEXTURE_CUBE_MAP, galaxyTexture_);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  for (int f = 0; f < 6; f++) {
    out.faces[f].resize(static_cast<size_t>(galaxyRes_) * galaxyRes_ * 3);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB, GL_FLOAT, out.faces[f].data());
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return out;
}
```

RGB, as `CubemapTarget::read()` reads. `measureGalaxy()` passes no accents: its count uniform is 0 by the code above.

- [ ] **Step 5: The wiring** -- in `src/core/sky.cpp`'s `bakeSky()`, add `#include "accents.h"` and replace the stars and glow lines:

```cpp
  // The accents from a stream of their own, so the stars before are as they
  // were; their members join the stars, their faint rest the glow.
  std::vector<Accent> accents = generateAccents(sky.scene.seed, galaxy, sky.accents);
  std::vector<Star> stars = generateStars(scene, galaxy, sky.stars, &band[0], &accents);
  band[1] = 0.49f * sky.stars.reach * sky.stars.reach;  // the typical distance, squared
  // The glow at the sky's own size unless told otherwise: its dust has detail
  // down to the texel. Exposed for where it is seen from. The accents' haze
  // in star-flux units, as the stars are drawn, into the glow's.
  float accentScale = sky.look.galaxyGlow > 0.0f
                          ? kStarFluxUnit * sky.look.starBrightness / sky.look.galaxyGlow
                          : 0.0f;
  baker.bakeGalaxy(galaxy, sky.look.reddening, sky.galaxyRes > 0 ? sky.galaxyRes : target.size(),
                   band, galaxyExposure(baker, galaxy, sky, band), 1.5f, &accents, accentScale);
```

- [ ] **Step 6: Run the tests**

Run: `cmake --build build -j $(nproc) 2>&1 | grep -E "error|warning"; ./build/test/test_galaxy | tail -3`
Expected: a `haze:` line with a positive first number, the other two near zero; no `check failed`.

Run: `ctest --test-dir build --output-on-failure 2>&1 | tail -4`
Expected: `100% tests passed` -- `test_look` unmoved, since `open-clusters` is 0.

Run, to see them at all: `./build/tools/atlas/atlas --out $SCRATCH/oc --seeds 1,11 --vantages own --set open-clusters=6 --set accent-near=0.7 --set nebula=on`
Expected: two sheets; knots of stars visible in the whole sky and some views.

- [ ] **Step 7: Commit**

```bash
git add src/core/shaders/galaxy.shader src/core/bake.h src/core/bake.cpp src/core/sky.cpp test/test_galaxy.cpp
git commit -F - <<'EOF'
Give open clusters the haze of their faint members

The light of a cluster's members too faint to draw is added where the
galaxy's march passes its distance, behind exactly the dust in front, and in
the stars' units, after the exposure and the knee as the stars' own light is.
Bakes now draw accents; open-clusters is still 0 by default.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 4: Mock-ups for the user (no commit)

**Files:** scratch only, outside the repository.

- [ ] **Step 1:** Render mock-ups with the atlas: seeds 1, 7, 11, 16 at the `own` vantage with the nebula on, each at `accent-near` 0, 0.4 and 0.8 (`--set open-clusters=6`); and seed 11 at `--macro galactic=1` and `-1`. Make per-sheet pages and an overview mosaic into `~/star-canopy-blind/open-clusters-mock-1/` with a `scores.md` whose table is `| sheet | question | reads as open clusters [yes/some/no] | good sky [yes/ok/no] | note |`, as `galactic-path-2` was made.
- [ ] **Step 2:** Look at each sheet yourself first for: clusters round or ball-like (a failure of the never-round rule); a haze visible over the nebula where a cluster is in front of it (the spec's checked limit); clusters appearing off the disc far out. Fix any before showing.
- [ ] **Step 3:** Point the user at `http://192.168.1.200:9876/`; wait for their notes. Tune `open-clusters`' default and `accent-near`'s from them.

---

### Task 5: Blind, and look 10

**Files:**
- Modify: `src/core/settings.h` (`openClusters` default, from Task 4), `src/core/project.h` (look 10 and its history line), `test/test_project.cpp`, `test/cli.cmake`, `test/test_look.cpp` (table via `--print`), `DESIGN.md` (§3.5, the look list), `ROADMAP.md` (tick *Open clusters*), `docs/studies/` (a new `accents.md` with the mock-ups and the blind result).

- [ ] **Step 1:** Blind pairs: `./build/tools/blind/blind --out ~/star-canopy-blind/open-clusters-1 --seeds 2,5,9,14,21,33,26,29 --around --dial open-clusters=0,N` (N the tuned default). Wait for the user's scores; unblind with `key.csv` only after every line is filled.
- [ ] **Step 2:** If preferred: set `int openClusters = N;`, add the history line `//  10  open clusters: knots of young stars where the galaxy's are, near by accent-near` and `kLookVersion = 10`; move `test_project`'s strings (`look = 10`, newer `look = 11`, a `fails("look = 9\n", "look = 10")`), `cli.cmake`'s `"look": 10`, and `test_look`'s table (rename to `kLook10`, record with `./build/test/test_look --print`). If not preferred: leave the default 0 and record the result; do not bump the look.
- [ ] **Step 3:** Docs as listed; `ctest --test-dir build --output-on-failure` green, quoted.
- [ ] **Step 4:** Commit by name: `git add` the files above; message `Scatter open clusters through the sky: look 10`, with the blind result in the body and the Co-Authored-By line.
