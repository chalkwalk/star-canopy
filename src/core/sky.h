#pragma once

#include "cubemap_target.h"
#include "settings.h"

#include <string>

namespace starcanopy {

// One whole bake, from the dials to a finished cubemap in `target`, whose size
// is the face's. Needs a current GL context. False, with the reason, if the
// shaders would not build.
bool bakeSky(const Settings& settings, CubemapTarget& target, std::string& error);

}  // namespace starcanopy
