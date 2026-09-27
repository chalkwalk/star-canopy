#pragma once

#include "project.h"

#include <string>
#include <vector>

namespace starcanopy {

// A project, rendered: baked, turned as its orientation says, and written in
// its formats, with a sidecar, name.json, that records what was made and where
// the key light is. Needs a current GL context. The paths written are added
// to `written`.
bool renderProject(const Project& project, std::vector<std::string>& written, std::string& error);

}  // namespace starcanopy
