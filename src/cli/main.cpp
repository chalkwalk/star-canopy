#include "cubemap.h"
#include "cubemap_target.h"
#include "gl_context.h"
#include "outputs.h"
#include "settings.h"
#include "sky.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

const char kUsage[] =
    "usage: starcanopy bake [--size N] [--set NAME=VALUE]... [--context K]\n"
    "                       [--out DIR] [--name NAME] [--formats F,F,...]\n"
    "       starcanopy dials\n"
    "\n"
    "bake    bakes a sky and writes it to DIR as NAME_px.exr .., NAME.ktx2 and so on\n"
    "  --size N         texels per face side (default 512)\n"
    "  --set NAME=VALUE a raw dial, repeatable; `starcanopy dials` lists them.\n"
    "                   These are overrides for scripting, not the interface: the\n"
    "                   macros that will steer a sky are made of them\n"
    "  --context K      auto (default): EGL, else a hidden window; egl: headless\n"
    "                   only; window: a hidden SDL window, which needs a display\n"
    "  --out DIR        existing directory to write into (default: write nothing)\n"
    "  --name NAME      the files' common name (default sky)\n"
    "  --formats LIST   any of exr-faces exr-equirect ktx2 png-faces png-cross\n"
    "                   png-equirect, comma separated (default exr-faces,ktx2).\n"
    "                   EXR and KTX2 are the linear HDR the bake made; PNG is\n"
    "                   derived from it through the display curve the look was\n"
    "                   judged by\n"
    "\n"
    "dials   lists every raw dial with its value and range\n";

int listDials() {
  using namespace starcanopy;
  int count = 0;
  const Dial* d = dials(count);
  Settings defaults;
  for (int i = 0; i < count; i++) {
    std::string range;
    if (d[i].choices) {
      for (int k = 0; k < d[i].choiceCount; k++) {
        range += std::string(k ? "|" : "") + d[i].choices[k];
      }
    } else if (!d[i].seed) {
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%g..%g", static_cast<double>(d[i].lo),
                    static_cast<double>(d[i].hi));
      range = buf;
    }
    std::printf("%-20s %-10s %-24s %s\n", d[i].name, dialValue(defaults, d[i]).c_str(),
                range.c_str(), d[i].help);
  }
  return 0;
}

int bake(int argc, char** argv) {
  using namespace starcanopy;
  int size = 512;
  ContextKind kind = ContextKind::Auto;
  std::string out, error;
  OutputRequest request;
  Settings settings;
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    bool hasValue = i + 1 < argc;
    if (arg == "--size" && hasValue) {
      size = std::atoi(argv[++i]);
    } else if (arg == "--set" && hasValue) {
      std::string assignment = argv[++i];
      size_t eq = assignment.find('=');
      if (eq == std::string::npos ||
          !setDial(settings, assignment.substr(0, eq), assignment.substr(eq + 1), error)) {
        std::fprintf(stderr, "starcanopy: %s\n",
                     eq == std::string::npos ? "--set wants NAME=VALUE" : error.c_str());
        return 2;
      }
    } else if (arg == "--context" && hasValue) {
      if (!parseContextKind(argv[++i], kind)) {
        std::fprintf(stderr, "starcanopy: unknown context '%s'\n", argv[i]);
        return 2;
      }
    } else if (arg == "--out" && hasValue) {
      out = argv[++i];
    } else if (arg == "--name" && hasValue) {
      request.name = argv[++i];
    } else if (arg == "--formats" && hasValue) {
      request.formats.clear();
      std::string list = argv[++i];
      for (size_t start = 0; start <= list.size();) {
        size_t comma = list.find(',', start);
        if (comma == std::string::npos) {
          comma = list.size();
        }
        if (comma > start) {
          request.formats.push_back(list.substr(start, comma - start));
        }
        start = comma + 1;
      }
    } else {
      std::fputs(kUsage, stderr);
      return 2;
    }
  }
  if (size < 1 || size > 16384) {
    std::fprintf(stderr, "starcanopy: --size must be 1 to 16384\n");
    return 2;
  }

  auto context = GlContext::create(kind, error);
  if (!context) {
    std::fprintf(stderr, "starcanopy: no OpenGL 3.3 context: %s\n", error.c_str());
    return 1;
  }
  std::printf("context: %s\n", context->description().c_str());

  auto start = std::chrono::steady_clock::now();
  CubemapTarget target(size);
  if (!bakeSky(settings, target, error)) {
    std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
    return 1;
  }
  Cubemap cubemap = target.read();
  double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::printf("baked seed %u, 6 x %d x %d, in %.2f s\n", settings.seed, size, size, seconds);

  if (!out.empty()) {
    request.directory = out;
    std::vector<std::string> written;
    if (!writeOutputs(cubemap, request, written, error)) {
      std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
      return 1;
    }
    for (const std::string& path : written) {
      std::printf("wrote %s\n", path.c_str());
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string command = argc >= 2 ? argv[1] : "";
  if (command == "bake") {
    return bake(argc, argv);
  }
  if (command == "dials") {
    return listDials();
  }
  bool help = command == "--help" || command == "-h";
  std::fputs(kUsage, help ? stdout : stderr);
  return help ? 0 : 2;
}
