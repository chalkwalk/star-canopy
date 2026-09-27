#include "tiles.h"

#include <algorithm>

namespace starcanopy {

std::vector<Tile> cubemapTiles(int size, int tileSize) {
  std::vector<Tile> tiles;
  if (size <= 0 || tileSize <= 0) {
    return tiles;
  }
  for (int face = 0; face < 6; face++) {
    for (int y = 0; y < size; y += tileSize) {
      for (int x = 0; x < size; x += tileSize) {
        tiles.push_back({face, x, y, std::min(tileSize, size - x), std::min(tileSize, size - y)});
      }
    }
  }
  return tiles;
}

}  // namespace starcanopy
