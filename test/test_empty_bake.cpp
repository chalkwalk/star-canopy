// StarCanopy -- the empty bake draws every texel, black, with no GL error.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "check.h"
#include "cubemap.h"
#include "empty_bake.h"
#include "gl_context.h"

#include <cstdio>
#include <string>

using namespace starcanopy;

int main(int argc, char** argv) {
  ContextKind kind = ContextKind::Auto;
  if (argc < 2 || !parseContextKind(argv[1], kind)) {
    std::printf("usage: test_empty_bake auto|egl|window\n");
    return 2;
  }
  std::string error;
  auto context = GlContext::create(kind, error);
  if (!context) {
    std::printf("no context: %s\n", error.c_str());
    // A hidden window needs a display, which a CI runner or an SSH session may
    // not have; that is a skip, not a failure. The headless path must work.
    return kind == ContextKind::HiddenWindow ? test::kSkipped : 1;
  }
  std::printf("context: %s\n", context->description().c_str());
  CHECK(kind == ContextKind::Auto || context->kind() == kind);

  // Not a multiple of the tile size, so the ragged last row and column of
  // tiles are exercised too.
  const int size = 300;
  EmptyBake bake(size);
  CHECK(bake.ok(error));
  if (!error.empty()) {
    std::printf("%s\n", error.c_str());
  }
  int steps = 0;
  while (bake.step()) {
    steps++;
  }
  CHECK(bake.progress() == 1.0f);
  CHECK(steps + 1 == static_cast<int>(cubemapTiles(size).size()));

  Cubemap cubemap = bake.target().read();
  CHECK(glGetError() == GL_NO_ERROR);
  CHECK(cubemap.size == size);
  for (int face = 0; face < 6; face++) {
    CHECK(cubemap.faces[face].size() == static_cast<size_t>(size) * size * 3);
    int unwritten = 0, nonBlack = 0;
    for (float v : cubemap.faces[face]) {
      unwritten += v == kUnwrittenTexel;
      nonBlack += v != 0.0f;
    }
    if (unwritten || nonBlack) {
      std::printf("face %s: %d unwritten, %d not black\n", faceName(face), unwritten, nonBlack);
    }
    CHECK(unwritten == 0);
    CHECK(nonBlack == 0);
  }
  return test::finish();
}
