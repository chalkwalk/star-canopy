#pragma once

#include <vector>

namespace starcanopy {

// A flat image of linear HDR RGB floats, first row at the top.
struct Image {
  int width = 0, height = 0;
  std::vector<float> rgb;
};

// src drawn into dst with its top left at (x0, y0), clipped to dst.
inline void paste(Image& dst, const Image& src, int x0, int y0) {
  for (int y = 0; y < src.height && y0 + y < dst.height; y++) {
    for (int x = 0; x < src.width && x0 + x < dst.width; x++) {
      for (int k = 0; k < 3; k++) {
        dst.rgb[3 * (static_cast<size_t>(y0 + y) * dst.width + x0 + x) + k] =
            src.rgb[3 * (static_cast<size_t>(y) * src.width + x) + k];
      }
    }
  }
}

}  // namespace starcanopy
