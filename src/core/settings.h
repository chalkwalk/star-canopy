#pragma once

#include "look.h"
#include "scene.h"

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
  int clusters = 2;
  float thickness = 0.06f;
  float fold = 0.18f;
  float foldScale = 1.6f;
  float keep = 0.7f;
  float holeScale = 1.3f;
  float outerSharpness = 6.0f;

  // Detail, and the distant nebulae.
  float detailScale = 5.0f;
  float detailGain = 0.55f;
  float erosion = 0.65f;
  float filament = 0.5f;
  float contrast = 1.5f;
  float hardness = 0.5f;
  float dust = 0.6f;
  float dustScale = 4.0f;
  float cavityDensity = 0.006f;
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
  float reflection = 0.2f;
  float anisotropy = 0.5f;
  float ionOpacity = 8.0f;
  float dustOpacity = 6.0f;


  // The bake.
  float exposure = 0.18f;
  float stepFrac = 0.15f;
  int maxSteps = 600;
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
};

// Every dial, in the order the labs' panel showed them.
const Dial* dials(int& count);

// Sets the dial called `name` from its text. False, with the reason, for an
// unknown name, a malformed value or one outside the dial's range.
bool setDial(Settings& s, const std::string& name, const std::string& value, std::string& error);

// The dial's current value as text, as setDial() reads it.
std::string dialValue(const Settings& s, const Dial& d);

// What one bake needs, made from the dials.
struct Sky {
  SceneParams scene;
  Look look;
};

Sky buildSky(const Settings& s);

}  // namespace starcanopy
