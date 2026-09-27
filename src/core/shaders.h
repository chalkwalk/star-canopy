#pragma once

namespace starcanopy {

// The source of src/core/shaders/<name>, or null if there is none. Defined in
// the file cmake/EmbedShaders.cmake generates.
const char* shaderSource(const char* name);

}  // namespace starcanopy
