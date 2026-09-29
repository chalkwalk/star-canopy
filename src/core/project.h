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
//   4  the galaxy's band made of stars, its glow only what is too faint to
//      draw with a haze kept; the bulge flattened; stars scaled from the
//      midplane, so an observer out of the disc sees fewer about them
//   5  the galaxy's type drawn by seed -- barred, grand design or flocculent
//      spiral -- its arms clear strands with dark space between them in the
//      outer disc, carried by knots of young stars that outreach the old disc
//   6  a quarter of galaxies lenticular: a large bulge, a lens, a bar in half,
//      rings, a smooth disc, dust only in rings about the centre
//   7  the galaxy's glow exposed for where it is seen from (galaxy-adapt): the
//      centre darker, the rim a little lighter; past the disc's edge, stars
//      counted from the disc, not from the empty midplane there
//   8  the observer placed by seed on a path through the galaxy, 2.2-3 scale
//      lengths out by default, moved by the galactic macro; lenticulars one in
//      eight, half with a lane of dust along the disc, their band grainier
//   9  the default sky more open (open +0.5's dials); the galactic routes
//      mostly outward, so remote is never over the bright disc
//
// This StarCanopy renders look 9 only, and refuses older looks with a reason.
constexpr int kLookVersion = 9;

// A project: how a sky is made again. Small, human-readable TOML:
//
//   look = 4                 # the look version it was made with
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
