#include "check.h"
#include "tiles.h"

#include <vector>

using namespace starcanopy;

namespace {

void checkCoverage(int size, int tileSize) {
  std::vector<int> hits(static_cast<size_t>(6) * size * size, 0);
  for (const Tile& t : cubemapTiles(size, tileSize)) {
    CHECK(t.face >= 0 && t.face < 6);
    CHECK(t.width >= 1 && t.width <= tileSize);
    CHECK(t.height >= 1 && t.height <= tileSize);
    CHECK(t.x >= 0 && t.x + t.width <= size);
    CHECK(t.y >= 0 && t.y + t.height <= size);
    for (int y = t.y; y < t.y + t.height; y++) {
      for (int x = t.x; x < t.x + t.width; x++) {
        hits[(static_cast<size_t>(t.face) * size + y) * size + x]++;
      }
    }
  }
  int wrong = 0;
  for (int h : hits) {
    wrong += h != 1;
  }
  if (wrong) {
    std::printf("size %d, tile %d: %d texels not covered exactly once\n", size, tileSize, wrong);
  }
  CHECK(wrong == 0);
}

}  // namespace

int main() {
  for (int size : {1, 2, 127, 128, 129, 255, 256, 300, 1024}) {
    checkCoverage(size, kTileSize);
  }
  checkCoverage(10, 3);
  checkCoverage(10, 20);

  // Whole tiles where the size allows, so the model's tiles match the labs'.
  CHECK(cubemapTiles(256).size() == 6 * 2 * 2);
  CHECK(cubemapTiles(257).size() == 6 * 3 * 3);
  CHECK(cubemapTiles(0).empty());
  CHECK(cubemapTiles(16, 0).empty());
  return test::finish();
}
