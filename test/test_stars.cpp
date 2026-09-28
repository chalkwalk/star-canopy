// The stars: the band's meets its budget and lies along the galaxy's plane,
// and neither the band nor the field stars run away when the observer is out
// of the disc -- measured against the midplane, not the empty ball about them.

#include "check.h"
#include "galaxy.h"
#include "scene.h"
#include "settings.h"
#include "stars.h"

#include <cmath>
#include <cstdio>

using namespace starcanopy;

namespace {

struct Census {
  size_t field, band;
  double nearPlane;   // the band's share within 10 degrees of the plane
  double brightest;   // the band's brightest star's flux
};

Census census(uint32_t seed, float height) {
  Settings s;
  s.seed = seed;
  s.galaxyHeight = height;
  Sky sky = buildSky(s);
  Scene scene = generateScene(sky.scene);
  Galaxy g = generateGalaxy(seed, sky.galaxy);
  StarParams none = sky.stars;
  none.bandCount = 0;
  Census c{};
  c.field = generateStars(scene, g, none).size();
  std::vector<Star> all = generateStars(scene, g, sky.stars);
  c.band = all.size() - c.field;
  size_t near = 0;
  for (size_t i = c.field; i < all.size(); i++) {
    float v[3];
    galaxyToFrame(g, all[i].dir, v);
    near += std::fabs(v[2]) < std::sin(10.0f * 3.14159265f / 180.0f);
    c.brightest = std::fmax(c.brightest, all[i].flux[1]);
  }
  c.nearPlane = c.band ? static_cast<double>(near) / c.band : 0.0;
  return c;
}

}  // namespace

int main() {
  StarParams defaults;
  for (uint32_t seed : {1u, 7u}) {
    Census mid = census(seed, 0.0f), above = census(seed, 3.0f);
    std::printf("seed %u: midplane %zu field, %zu band, %.0f%% within 10 deg, brightest %.3g; "
                "3 kpc up %zu field, %zu band, brightest %.3g\n",
                seed, mid.field, mid.band, 100.0 * mid.nearPlane, mid.brightest, above.field,
                above.band, above.brightest);
    // The budget, give or take the draw.
    CHECK(std::fabs(static_cast<double>(mid.band) / defaults.bandCount - 1.0) < 0.1);
    // Along the plane: most of the band within ten degrees of it.
    CHECK(mid.nearPlane > 0.6);
    // Out of the disc, fewer field stars about the observer, and the band no
    // brighter than from within it.
    CHECK(above.field < mid.field / 2);
    CHECK(above.brightest <= mid.brightest * 2.0);
  }
  return test::finish();
}
