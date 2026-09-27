#pragma once

#include <vector>

namespace starcanopy {

// A flat image of linear HDR RGB floats, first row at the top.
struct Image {
  int width = 0, height = 0;
  std::vector<float> rgb;
};

}  // namespace starcanopy
