#pragma once

#include "outputs.h"
#include "settings.h"

#include <cstdint>
#include <string>

namespace starcanopy {

// The look this StarCanopy renders (DESIGN.md §7). A change that alters any
// sky's pixels is a new look version; a project records the look it was made
// with, and a StarCanopy that cannot render that look says so rather than
// render it differently (PRINCIPLES §7).
constexpr int kLookVersion = 1;

// A project: how a sky is made again. Small, human-readable TOML:
//
//   look = 1                 # the look version it was made with
//   seed = 7
//   style = "mass"           # mass | shell
//
//   [macros]                 # none exist yet
//
//   [overrides]              # raw dials, for scripting (fence #7)
//   density = 1.2
//
//   [orientation]            # degrees; see rotation() in sample.h
//   yaw = 0.0
//   pitch = 0.0
//   roll = 0.0
//
//   [output]
//   size = 2048              # texels per face
//   directory = "sky"        # relative to the project file
//   name = "sky"
//   formats = ["exr-faces", "ktx2", "png-faces"]
//   equirect_width = 0       # 0: four times the size
//
// Every key but look is optional. Unknown keys are errors: a typo silently
// ignored is a sky silently different from the one meant.
struct Project {
  int look = kLookVersion;
  Settings settings;
  float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
  int size = 2048;
  OutputRequest output;
};

bool loadProject(const std::string& path, Project& project, std::string& error);

// A new project's text, for `starcanopy new`.
std::string projectText(uint32_t seed, const std::string& style, const std::string& name);

}  // namespace starcanopy
