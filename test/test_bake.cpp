// A whole bake, small: every texel drawn, finite and never negative, the sky not
// empty, no GL error, and the same dials baking the same pixels twice
// (PRINCIPLES §7: nothing in a render depends on anything but the project).

#include "check.h"
#include "cubemap.h"
#include "cubemap_target.h"
#include "gl_context.h"
#include "settings.h"
#include "sky.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace starcanopy;

int main(int argc, char** argv) {
  ContextKind kind = ContextKind::Auto;
  if (argc < 2 || !parseContextKind(argv[1], kind)) {
    std::printf("usage: test_bake auto|egl|window\n");
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

  // Not a multiple of the tile size, so ragged edge tiles are exercised.
  const int size = 136;
  Settings settings;
  settings.seed = 7;
  Cubemap first;
  for (int run = 0; run < 2; run++) {
    CubemapTarget target(size);
    bool baked = bakeSky(settings, target, error);
    if (!baked) {
      std::printf("%s\n", error.c_str());
    }
    CHECK(baked);
    Cubemap cubemap = target.read();
    CHECK(glGetError() == GL_NO_ERROR);
    if (run == 0) {
      double sum = 0.0;
      for (int face = 0; face < 6; face++) {
        int unwritten = 0, bad = 0;
        for (float v : cubemap.faces[face]) {
          unwritten += v == kUnwrittenTexel;
          bad += !std::isfinite(v) || v < 0.0f;
          sum += v;
        }
        if (unwritten || bad) {
          std::printf("face %s: %d unwritten, %d negative or not finite\n", faceName(face),
                      unwritten, bad);
        }
        CHECK(unwritten == 0);
        CHECK(bad == 0);
      }
      CHECK(sum > 0.0);
      first = cubemap;
    } else {
      CHECK(cubemap.faces == first.faces);
    }
  }
  return test::finish();
}
