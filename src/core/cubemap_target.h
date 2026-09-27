// StarCanopy -- a cubemap on the GPU that bakes draw into and read back from.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

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

private:
  int size_;
  GLuint texture_ = 0;
  GLuint framebuffer_ = 0;
};

}  // namespace starcanopy
