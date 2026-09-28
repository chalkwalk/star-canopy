#include "sky.h"

#include "bake.h"

#include <cmath>

namespace starcanopy {

namespace {

// The dust in front of each distant nebula: the galaxy's, along the line to its
// centre, reddened by the same law as the stars behind it. What makes one read
// as behind the main nebula rather than laid over it -- dimmer and warmer the
// further through the disc it lies.
void veilDistant(Scene& scene, const Galaxy& galaxy, const Sky& sky) {
  for (int i = 1; i < scene.bubbleCount; i++) {
    Bubble& b = scene.bubble[i];
    float d = std::sqrt(b.center[0] * b.center[0] + b.center[1] * b.center[1] +
                        b.center[2] * b.center[2]);
    float dir[3] = {b.center[0] / d, b.center[1] / d, b.center[2] / d}, offset[3];
    galaxyToFrame(galaxy, dir, offset);
    for (float& o : offset) {
      o *= d * sky.stars.kpcPerSkyUnit;
    }
    float tau = sky.distantDust * dustDepth(galaxy, offset);
    for (int k = 0; k < 3; k++) {
      b.veil[k] = std::exp(-tau * sky.look.reddening[k]);
    }
  }
}

}  // namespace

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
  veilDistant(scene, galaxy, sky);
  // The glow at the sky's own size unless told otherwise: its dust has detail
  // down to the texel.
  baker.bakeGalaxy(galaxy, sky.look.reddening, sky.galaxyRes > 0 ? sky.galaxyRes : target.size());
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
