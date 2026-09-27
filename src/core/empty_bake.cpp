// StarCanopy -- the empty bake: a black cubemap, drawn the way the model will be.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "empty_bake.h"

namespace starcanopy {

EmptyBake::EmptyBake(int size) : target_(size), tiles_(cubemapTiles(size)) {
  program_.build("fullscreen.vert", "empty_bake.frag", buildError_);
  // Core profile draws need a bound vertex array, even with no attributes.
  glGenVertexArrays(1, &vertexArray_);
}

EmptyBake::~EmptyBake() {
  glDeleteVertexArrays(1, &vertexArray_);
}

bool EmptyBake::ok(std::string& error) const {
  error = buildError_;
  return buildError_.empty();
}

bool EmptyBake::step() {
  if (!program_.id() || next_ >= tiles_.size()) {
    return false;
  }
  const Tile& tile = tiles_[next_++];
  target_.bindFace(tile.face);
  glUseProgram(program_.id());
  glBindVertexArray(vertexArray_);
  glEnable(GL_SCISSOR_TEST);
  glScissor(tile.x, tile.y, tile.width, tile.height);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisable(GL_SCISSOR_TEST);
  // Wait for the tile, so the driver never holds a queue of them that together
  // run long enough to trip its watchdog.
  glFinish();
  return next_ < tiles_.size();
}

float EmptyBake::progress() const {
  return tiles_.empty() ? 1.0f : static_cast<float>(next_) / static_cast<float>(tiles_.size());
}

}  // namespace starcanopy
