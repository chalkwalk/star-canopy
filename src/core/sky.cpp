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

// The galaxy's exposure for where it is seen from, as an eye or a camera
// adapts: from the brightness most of the sky reaches (its 90th percentile),
// against that at the Sun's place, followed part of the way -- so from the
// centre, where the whole sky glows, its structure shows under the knee
// instead of burning to grey, and from the rim the faint local disc shows
// instead of black. Bounded: from far out, the galaxy is a small bright thing
// in a dark sky and is not to be blown out finding light in the dark. Measured
// at one small size, so a preview has the export's exposure (PRINCIPLES §9);
// the numbers are in docs/studies/atlas.md.
float galaxyExposure(Baker& baker, const Galaxy& galaxy, const Sky& sky, const float band[3]) {
  if (sky.galaxyAdapt <= 0.0f) {
    return 1.0f;
  }
  const float reference = 0.12f, least = 0.25f, most = 2.5f;
  float m[4];
  baker.measureGalaxy(galaxy, sky.look.reddening, band, m);
  float e = powf(reference / fmaxf(m[1], 1e-9f), sky.galaxyAdapt);
  return fminf(fmaxf(e, least), most);
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
  // The stars come from the whole scene, clusters and all, even with the nebula
  // off: they are the same sky, seen without its gas. First, since the glow is
  // only the stars too faint to be drawn.
  float band[3] = {0.0f, 0.0f, sky.galaxyHaze / galaxy.grain};
  std::vector<Star> stars = generateStars(scene, galaxy, sky.stars, &band[0]);
  band[1] = 0.49f * sky.stars.reach * sky.stars.reach;  // the typical distance, squared
  // The glow at the sky's own size unless told otherwise: its dust has detail
  // down to the texel. Exposed for where it is seen from.
  baker.bakeGalaxy(galaxy, sky.look.reddening, sky.galaxyRes > 0 ? sky.galaxyRes : target.size(),
                   band, galaxyExposure(baker, galaxy, sky, band));
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
