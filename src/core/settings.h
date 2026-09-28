#pragma once

#include "galaxy.h"
#include "look.h"
#include "scene.h"
#include "stars.h"

#include <cstdint>
#include <string>

namespace starcanopy {

// The raw parameters: every dial the model has, with the values the sky was
// judged at in the labs. These are not an interface (PRINCIPLES §5, fence #7):
// they are reachable from the command line as overrides, for whoever knows
// exactly what they are doing, and the macros will be made of them.
struct Settings {
  uint32_t seed = 1;

  // What is where.
  float viewerOffset = 0.55f;
  float clusterOffset = 0.85f;
  float luminosity = 1.0f;
  float fold = 0.18f;
  float foldScale = 1.6f;
  float keep = 0.7f;
  float holeScale = 1.3f;

  // Detail, and the distant nebulae.
  float detailScale = 5.0f;
  float detailGain = 0.55f;
  float erosion = 0.65f;
  float filament = 0.5f;
  float contrast = 1.5f;
  float hardness = 0.5f;
  float distantDensity = 1.0f;
  float distantCavity = 0.008f;
  float distantBlister = 0.45f;
  float distantDust = 4.0f;
  float massInner = 0.75f;
  float massLobes = 0.4f;
  float massScale = 1.5f;
  float massDensity = 6.0f;
  int massClusters = 1;
  float massCavity = 0.0f;
  float massFine = 1.0f;
  float massBillow = 1.0f;
  float massEdge = 0.0f;
  float massEdgePatch = 0.0f;
  float graze = 3.0f;
  float massBlister = 0.3f;
  float massClusterSize = 0.15f;
  float massWarp = 0.25f;
  float fillShadow = 15.0f;
  float fill = 0.1f;
  int distantCount = 3;
  float distantMinDegrees = 3.0f;
  float distantMaxDegrees = 14.0f;

  // The light.
  float density = 1.0f;
  float sigma = 8.0f;
  float oxygenThreshold = 20.0f;
  float lineO = 1.0f;
  float lineH = 1.0f;
  float lineS = 0.5f;
  int lineColors = 0;  // natural
  int grade = 1;  // auto
  float gradeStrength = 1.0f;
  float gradeChroma = 1.0f;
  float haze = 0.004f;
  float shoulder = 0.8f;
  int hueType = 0;  // auto
  int paletteFamily = 0;  // auto
  float gradeGalaxy = 0.2f;
  float gradeStars = 0.6f;
  float reflection = 0.2f;
  float anisotropy = 0.5f;
  float ionOpacity = 8.0f;

  // The stars and the galaxy.
  float starBrightness = 0.55f;
  int starCount = 30000;
  int bandStars = 150000;
  float galaxyHaze = 0.3f;
  float starReach = 1.5f;
  float nebulaScale = 0.08f;
  float clusterStars = 1.0f;
  int young = 40;
  float starHalo = 0.1f;
  float starHaloDegrees = 0.4f;
  // Diffraction spikes are off: baked into a sky they read as a telescope's
  // artefact, not as a star.
  float spike = 0.0f;
  float spikeFlux = 5000.0f;
  int nebula = 1;
  int galaxyStyle = 0;  // barred spiral
  float galaxyRadius = 3.5f;
  float galaxyHeight = 0.0f;
  float galaxyGlow = 1.5f;
  float galaxyDust = 1.0f;
  float galaxyWarp = 1.0f;
  float galaxyWaves = 1.0f;
  int externalGalaxies = 4;
  int galaxyRes = 0;  // the sky's own size

  // The bake.
  float exposure = 0.18f;
  float denoise = 0.35f;
  int supersample = 2;
  float stepFrac = 0.15f;
  int maxSteps = 600;
  int lightRes = 96;
  int lightSteps = 48;
};

// One raw parameter, as the command line names it.
struct Dial {
  const char* name;
  const char* help;
  float Settings::*real = nullptr;
  int Settings::*integer = nullptr;
  uint32_t Settings::*seed = nullptr;
  float lo = 0.0f, hi = 0.0f;
  // A choice among names, stored as an index in `integer`.
  const char* const* choices = nullptr;
  int choiceCount = 0;
  // Stepped by factors rather than by amounts: a density, a size, a scale,
  // whose natural steps are ratios. Such a dial may still allow 0, as off,
  // and then `floor` is its smallest value above zero worth stepping from.
  bool geometric = false;
  float floor = 0.0f;
  // The macro this dial belongs to, or null. A dial that exists only to serve
  // one macro is not a raw parameter: it is set through that macro, and not
  // listed, overridden or set on its own, so the dials stay the model's.
  const char* owner = nullptr;
};

// Every dial, in the order the labs' panel showed them.
const Dial* dials(int& count);

// Sets the dial called `name` from its text. False, with the reason, for an
// unknown name, a dial a macro owns, a malformed value or one outside the
// dial's range.
bool setDial(Settings& s, const std::string& name, const std::string& value, std::string& error);

// The dial's current value as text, as setDial() reads it.
std::string dialValue(const Settings& s, const Dial& d);

// What one bake needs, made from the dials.
struct Sky {
  SceneParams scene;
  float distantDust;  // scales the dust in front of the distant nebulae
  Look look;
  GalaxyParams galaxy;
  StarParams stars;
  int galaxyRes;
  float galaxyHaze;  // the galaxy's glow kept where its stars are drawn
  bool nebula;
};

Sky buildSky(const Settings& s);

}  // namespace starcanopy
