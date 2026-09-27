#pragma once

#include "bake.h"
#include "cubemap_target.h"
#include "settings.h"

#include <string>

namespace starcanopy {

// One whole bake, from the dials to a finished cubemap in `target`, whose size
// is the face's. Needs a current GL context. False, with the reason, if the
// shaders would not build.
bool bakeSky(const Settings& settings, CubemapTarget& target, std::string& error);

// The same with a baker that outlives the bake, for many bakes in a row: its
// shaders are built once, where bakeSky() above builds them every time.
void bakeSky(Baker& baker, const Settings& settings, CubemapTarget& target);

// The direction of the sky's key light, a unit cubemap lookup vector: the
// brightest cluster of the main nebula, the light the sky's form is lit by. A
// scene's directional light matches the sky when it shines from here
// (PRINCIPLES §14).
void keyLight(const Settings& settings, float dir[3]);

}  // namespace starcanopy
