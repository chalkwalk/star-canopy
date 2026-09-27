// StarCanopy -- Portable Float Map, the interim HDR writer.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

namespace starcanopy {

// PFM is linear float RGB with a three-line text header and no dependencies,
// which is all a first bake needs to be looked at (with any HDR viewer). It is
// a stopgap: OpenEXR and KTX2 are the real HDR outputs (DESIGN.md §8).
//
// Rows are written as given, first row at the bottom of the image, which is
// both PFM's order and GL's.
bool writePfm(const std::string& path, int width, int height, const std::vector<float>& rgb,
              std::string& error);

bool readPfm(const std::string& path, int& width, int& height, std::vector<float>& rgb,
             std::string& error);

}  // namespace starcanopy
