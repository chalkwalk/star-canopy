// StarCanopy -- the empty bake: a black cubemap, drawn the way the model will be.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "cubemap_target.h"
#include "shader.h"
#include "tiles.h"

#include <string>
#include <vector>

namespace starcanopy {

// Draws a black sky tile by tile, finishing the GPU's work after each, exactly
// as the model's bake will: it proves the context, the tiling and the readback
// before there is a model to get wrong. Needs a current GL context.
class EmptyBake {
public:
  explicit EmptyBake(int size);
  ~EmptyBake();
  EmptyBake(const EmptyBake&) = delete;
  EmptyBake& operator=(const EmptyBake&) = delete;

  // False, with the reason, if the shaders would not build.
  bool ok(std::string& error) const;

  // Draws one tile. Returns whether any remain. One tile per call, so an
  // interface can bake between frames and stay responsive.
  bool step();

  float progress() const;
  const CubemapTarget& target() const { return target_; }

private:
  CubemapTarget target_;
  Program program_;
  std::string buildError_;
  std::vector<Tile> tiles_;
  size_t next_ = 0;
  GLuint vertexArray_ = 0;
};

}  // namespace starcanopy
