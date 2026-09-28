#pragma once

#include "macros.h"
#include "outputs.h"
#include "settings.h"

#include <cstdint>
#include <string>

namespace starcanopy {

// The look this StarCanopy renders (DESIGN.md §7). A change that alters any
// sky's pixels is a new look version; a project records the look it was made
// with, and a StarCanopy that cannot render that look says so rather than
// render it differently (PRINCIPLES §7).
//
//   1  the lift from the labs: a mass or a thin shell, distant shells
//   2  the shell retired: every nebula a mass, the distant ones lit from beside
//      them (docs/studies/distant.md); a mass sky's main nebula as in look 1
//   3  the galaxy's dust at every scale, by seed more or less clumped, its glow
//      baked at the sky's own size; the observer on the midplane
//      (docs/studies/atlas.md)
//
// This StarCanopy renders look 3 only, and refuses older looks with a reason.
constexpr int kLookVersion = 3;

// A project: how a sky is made again. Small, human-readable TOML:
//
//   look = 3                 # the look version it was made with
//   seed = 7
//   style = "mass"           # mass; the sparse compositions are to come
//
//   [macros]                 # -1..1, 0 the seed's own sky: `starcanopy macros`
//   open = 0.4
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
  // The dials' base values: the defaults, and the overrides over them. The
  // macros move them from there; resolved() is the sky.
  Settings settings;
  MacroValues macros;
  float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
  int size = 2048;
  OutputRequest output;

  Settings resolved() const { return resolveMacros(settings, macros); }
};

bool loadProject(const std::string& path, Project& project, std::string& error);

// A new project's text, for `starcanopy new`.
std::string projectText(uint32_t seed, const std::string& name);

}  // namespace starcanopy
