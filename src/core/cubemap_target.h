#pragma once

#include "cubemap.h"

#include <glad/gl.h>

namespace starcanopy {

// Every texel starts as this, which no bake writes (radiance is never
// negative), so a tile that was never drawn shows in the readback rather than
// passing for black sky.
constexpr float kUnwrittenTexel = -1.0f;

// RGBA16F faces with a framebuffer to draw one face at a time. Needs a
// current GL context for its whole life.
class CubemapTarget {
public:
  explicit CubemapTarget(int size);
  ~CubemapTarget();
  CubemapTarget(const CubemapTarget&) = delete;
  CubemapTarget& operator=(const CubemapTarget&) = delete;

  int size() const { return size_; }
  GLuint texture() const { return texture_; }

  // Binds the framebuffer with `face` attached and the viewport covering it.
  void bindFace(int face);

  Cubemap read() const;

  // Alpha: the transmittance of the gas along each texel's line of sight, 0
  // where it hides everything behind it, 1 where it hides nothing. Six faces,
  // one float a texel.
  std::array<std::vector<float>, 6> readTransmittance() const;

private:
  int size_;
  GLuint texture_ = 0;
  GLuint framebuffer_ = 0;
};

}  // namespace starcanopy
