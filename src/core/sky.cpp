#include "sky.h"

#include "bake.h"

#include <cmath>

namespace starcanopy {

bool bakeSky(const Settings& settings, CubemapTarget& target, std::string& error) {
  Baker baker;
  if (!baker.ok(error)) {
    return false;
  }
  bakeSky(baker, settings, target);
  return true;
}

void bakeSky(Baker& baker, const Settings& settings, CubemapTarget& target) {
  Sky sky = buildSky(settings);
  Scene scene = generateScene(sky.scene);
  Galaxy galaxy = generateGalaxy(sky.scene.seed, sky.galaxy);
  baker.bakeGalaxy(galaxy, sky.look.reddening, sky.galaxyRes);
  // The stars come from the whole scene, clusters and all, even with the nebula
  // off: they are the same sky, seen without its gas.
  std::vector<Star> stars = generateStars(scene, galaxy, sky.stars);
  if (!sky.nebula) {
    scene.bubbleCount = 0;
  }
  baker.bakeLight(scene, sky.look);
  baker.begin(target, scene, stars, sky.look);
  while (baker.step()) {
  }
}

void keyLight(const Settings& settings, float dir[3]) {
  Scene scene = generateScene(buildSky(settings).scene);
  // The first cluster is the brightest by construction (scene.cpp).
  const Bubble& main = scene.bubble[0];
  bubbleToSky(main, main.cluster[0].pos, dir);
  float length = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
  for (int k = 0; k < 3; k++) {
    dir[k] /= length;
  }
}

}  // namespace starcanopy
