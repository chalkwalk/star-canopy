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
  baker.bakeLight(scene, sky.look);
  baker.begin(target, scene, sky.look);
  while (baker.step()) {
  }
  return true;
}

}  // namespace starcanopy
