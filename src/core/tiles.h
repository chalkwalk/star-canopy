// StarCanopy -- cutting cubemap faces into tiles, one draw each.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

namespace starcanopy {

// Texels per side of a face tile. A single long draw trips the GPU driver's
// watchdog, which resets the GPU and can take the desktop with it; at this size
// one heavy tile of the model is milliseconds, not seconds, on a small
// integrated GPU (the labs' value).
constexpr int kTileSize = 128;

struct Tile {
  int face;  // 0-5, in GL order: +x -x +y -y +z -z
  int x, y;  // lower-left texel, GL convention
  int width, height;
};

// Every texel of six size x size faces, in tiles no larger than tileSize a
// side, face by face and row by row from the bottom. The last row and column
// of a face are narrower when tileSize does not divide size.
std::vector<Tile> cubemapTiles(int size, int tileSize = kTileSize);

}  // namespace starcanopy
