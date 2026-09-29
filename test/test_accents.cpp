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
