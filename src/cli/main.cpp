// StarCanopy -- the command line.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cubemap.h"
#include "empty_bake.h"
#include "gl_context.h"
#include "pfm.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

const char kUsage[] =
    "usage: starcanopy bake-empty [--size N] [--context auto|egl|window] [--out DIR]\n"
    "\n"
    "Bakes a black cubemap through the same context, tiling and readback the\n"
    "model will use, and writes its faces as DIR/px.pfm ... DIR/nz.pfm (linear\n"
    "float). A check of the machine's GL, until `starcanopy render` exists.\n"
    "\n"
    "  --size N      texels per face side (default 256)\n"
    "  --context K   auto (default): EGL, else a hidden window; egl: headless\n"
    "                only; window: a hidden SDL window, which needs a display\n"
    "  --out DIR     existing directory to write into (default: write nothing)\n";

int bakeEmpty(int argc, char** argv) {
  using namespace starcanopy;
  int size = 256;
  ContextKind kind = ContextKind::Auto;
  std::string out;
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    bool hasValue = i + 1 < argc;
    if (arg == "--size" && hasValue) {
      size = std::atoi(argv[++i]);
    } else if (arg == "--context" && hasValue) {
      if (!parseContextKind(argv[++i], kind)) {
        std::fprintf(stderr, "starcanopy: unknown context '%s'\n", argv[i]);
        return 2;
      }
    } else if (arg == "--out" && hasValue) {
      out = argv[++i];
    } else {
      std::fputs(kUsage, stderr);
      return 2;
    }
  }
  if (size < 1 || size > 16384) {
    std::fprintf(stderr, "starcanopy: --size must be 1 to 16384\n");
    return 2;
  }

  std::string error;
  auto context = GlContext::create(kind, error);
  if (!context) {
    std::fprintf(stderr, "starcanopy: no OpenGL 3.3 context: %s\n", error.c_str());
    return 1;
  }
  std::printf("context: %s\n", context->description().c_str());

  auto start = std::chrono::steady_clock::now();
  EmptyBake bake(size);
  if (!bake.ok(error)) {
    std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
    return 1;
  }
  while (bake.step()) {
  }
  Cubemap cubemap = bake.target().read();
  double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::printf("baked 6 x %d x %d in %.3f s\n", size, size, seconds);

  if (!out.empty()) {
    for (int face = 0; face < 6; face++) {
      std::string path = out + "/" + faceName(face) + ".pfm";
      if (!writePfm(path, size, size, cubemap.faces[face], error)) {
        std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
        return 1;
      }
      std::printf("wrote %s\n", path.c_str());
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc >= 2 && std::string(argv[1]) == "bake-empty") {
    return bakeEmpty(argc, argv);
  }
  std::fputs(kUsage, argc >= 2 && std::string(argv[1]) == "--help" ? stdout : stderr);
  return argc >= 2 && std::string(argv[1]) == "--help" ? 0 : 2;
}
