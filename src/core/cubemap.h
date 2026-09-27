#pragma once

#include <array>
#include <vector>

namespace starcanopy {

// Linear HDR radiance, RGB floats (PRINCIPLES §8). Faces are in GL order
// (+x -x +y -y +z -z) and rows are as GL stores them, the first row at the
// face's t = -1 edge. How faces are oriented and named in a file is the
// writer's business, not the cubemap's.
struct Cubemap {
  int size = 0;
  std::array<std::vector<float>, 6> faces;
};

// Face names in the common engine naming (DESIGN.md §8), in GL order.
const char* faceName(int face);

}  // namespace starcanopy
