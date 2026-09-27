// StarCanopy -- shader sources compiled into the binary.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace starcanopy {

// The source of src/core/shaders/<name>, or null if there is none. Defined in
// the file cmake/EmbedShaders.cmake generates.
const char* shaderSource(const char* name);

}  // namespace starcanopy
