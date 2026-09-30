// The accents: open clusters placed where the galaxy's young stars are, as many
// as the dial asks about a place in the band, nearer as accent-near rises, the
// far ones in the disc; none where there is no disc to be born in.

#include "accents.h"
#include "check.h"
#include "galaxy.h"
#include "scene.h"
#include "settings.h"
#include "stars.h"

#include <algorithm>
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
      // Compact: a cluster's stars close enough together to read as one
      // object; at 3-12 pc they were a thin scatter lost in the field.
      CHECK(x.association || x.radius < 0.009f);
      // Never so near it fills the screen, however strong the near bias: its
      // stars spread to about four radii, at most 1/15 of a 75-degree view's
      // height, about 3 degrees; a sixth of it read as a pasted patch.
      CHECK(4.0f * x.radius / x.distance <= 0.055f);
      CHECK(std::fabs(std::sqrt(x.dir[0] * x.dir[0] + x.dir[1] * x.dir[1] + x.dir[2] * x.dir[2]) -
                      1.0f) < 1e-4f);
      if (x.distance > 1.0f) {
        far++;
        farInDisc += std::fabs(x.pos[2]) < 0.5f;
      }
    }
    for (const Accent& x : generateAccents(seed, gs, close)) {
      CHECK(4.0f * x.radius / x.distance <= 0.055f);
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
  // Near enough to find at 1: a cluster stands out only as its members are
  // among the brightest stars about it, which, a few kiloparsecs off in the
  // band, they are not (the mock-ups at game resolution).
  CHECK(nearTotal / nearCount < 1.0);
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
    // On the field stars' scale: only a cluster's brightest members are drawn --
    // a few tens, as the Pleiades show a handful to the eye out of a thousand --
    // and its faint majority is its haze, even with no limit. Thousands drawn
    // made a solid white ball, a globular's look, not an open cluster's.
    CHECK(members >= 10 && members <= 400);
    CHECK(list[0].glow[0] > 0.0f && list[0].glow[1] > 0.0f && list[0].glow[2] > 0.0f);

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

    // A brightness hierarchy: a few luminous members carry it, the rest small
    // and faint -- as a handful of bright stars carry the Pleiades. Many equal
    // medium-bright members were all fat points of a size.
    // And a concentrated core thinning into a sparse halo, emerging from the
    // field rather than sitting on it as a disc.
    {
      size_t start = generateStars(scene, clean, sp).size();
      std::vector<float> fluxes;
      int core = 0, halo = 0;
      float size = one.radius / one.distance;
      for (size_t i = start; i < bare.size(); i++) {
        fluxes.push_back(bare[i].flux[1]);
        float c = bare[i].dir[0] * one.dir[0] + bare[i].dir[1] * one.dir[1] + bare[i].dir[2] * one.dir[2];
        float angle = std::acos(std::fmin(c, 1.0f));
        core += angle < size;
        halo += angle > 1.5f * size;
      }
      std::sort(fluxes.begin(), fluxes.end());
      float top = fluxes.back(), median = fluxes[fluxes.size() / 2];
      int bright = 0;
      for (float f : fluxes) {
        bright += f > 0.25f * top;
      }
      double n = static_cast<double>(fluxes.size());
      std::printf("hierarchy: %d bright of %zu, median %.3g of the brightest; core %.0f%%, halo %.0f%%\n",
                  bright, fluxes.size(), median / top, 100.0 * core / n, 100.0 * halo / n);
      CHECK(bright >= 1 && bright <= 10);
      CHECK(median < 0.05f * top);
      // Most small and faint: the median member no brighter than a star of the
      // field's faintest luminosity at the cluster's distance. Every member at
      // least that bright, and the young ones four times it, made fat points.
      float typical = 0.7f * sp.reach, unit = typical * typical / (one.distance * one.distance);
      std::printf("median member %.3g against a unit star's %.3g\n", median, unit);
      CHECK(median <= 1.2f * unit);
      CHECK(core >= 0.5 * n && halo >= 0.1 * n);
    }
    // Its haze, the faint majority, 30% of the light its drawn members give:
    // enough to bind them into a patch; as much again was a fog over them.
    std::printf("haze %.4g against members %.4g\n", list2[0].glow[1], fb);
    CHECK(std::fabs(list2[0].glow[1] / fb - 0.3) < 0.01);

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
  return test::finish();
}
