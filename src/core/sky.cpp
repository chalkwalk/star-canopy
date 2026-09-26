#include "sky.h"

#include "bake.h"

namespace starcanopy {

bool bakeSky(const Settings& settings, CubemapTarget& target, std::string& error) {
  Baker baker;
  if (!baker.ok(error)) {
    return false;
  }
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
  return true;
}

}  // namespace starcanopy
