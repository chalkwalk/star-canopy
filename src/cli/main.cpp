#include "gl_context.h"
#include "macros.h"
#include "project.h"
#include "render.h"
#include "settings.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

const char kUsage[] =
    "usage: starcanopy new PROJECT.toml [--seed N]\n"
    "       starcanopy render PROJECT.toml [--macro NAME=VALUE]... [--set NAME=VALUE]...\n"
    "                         [--size N] [--out DIR] [--context K]\n"
    "       starcanopy macros\n"
    "       starcanopy dials\n"
    "\n"
    "new     writes a new project, to render and to edit\n"
    "render  bakes a project's sky and writes it in the project's formats,\n"
    "        with NAME.json beside the images: the key light's direction and\n"
    "        what was made\n"
    "  --macro NAME=VALUE a macro, -1 to 1, over the project's own, repeatable\n"
    "  --set NAME=VALUE a raw dial's base, over the project's own, repeatable. For\n"
    "                   scripting, not steering: `starcanopy dials` lists them\n"
    "  --size N         texels per face, over the project's\n"
    "  --out DIR        where to write, over the project's\n"
    "  --context K      auto (default): EGL, else a hidden window; egl: headless\n"
    "                   only; window: a hidden SDL window, which needs a display\n"
    "macros  lists the macros: what each does, and the dials it moves\n"
    "dials   lists every raw dial with its default and range\n";

int listMacros() {
  using namespace starcanopy;
  int count = 0;
  const Macro* m = macros(count);
  for (int i = 0; i < count; i++) {
    std::printf("%s\n  -1 %s .. 1 %s: %s\n", m[i].name, m[i].opposite, m[i].name, m[i].help);
    for (int k = 0; k < m[i].bindingCount; k++) {
      const Binding& b = m[i].bindings[k];
      const char* unit = b.pull == Pull::Octaves ? " octaves" : "";
      std::printf("    %-20s %+g%s at 1, %+g%s at -1\n", b.dial, static_cast<double>(b.up), unit,
                  static_cast<double>(b.down), unit);
    }
  }
  return 0;
}

int listDials() {
  using namespace starcanopy;
  int count = 0;
  const Dial* d = dials(count);
  Settings defaults;
  for (int i = 0; i < count; i++) {
    if (d[i].owner) {
      continue;  // set through its macro: `starcanopy macros`
    }
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

int newProject(int argc, char** argv) {
  using namespace starcanopy;
  if (argc < 3) {
    std::fputs(kUsage, stderr);
    return 2;
  }
  std::string path = argv[2];
  uint32_t seed = 1;
  for (int i = 3; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--seed" && i + 1 < argc) {
      Settings s;
      std::string error;
      if (!setDial(s, "seed", argv[++i], error)) {
        std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
        return 2;
      }
      seed = s.seed;
    } else {
      std::fputs(kUsage, stderr);
      return 2;
    }
  }
  if (std::filesystem::exists(path)) {
    std::fprintf(stderr, "starcanopy: %s exists; not overwriting it\n", path.c_str());
    return 1;
  }
  std::string name = std::filesystem::path(path).stem().string();
  std::ofstream out(path);
  out << projectText(seed, name.empty() ? "sky" : name);
  if (!out) {
    std::fprintf(stderr, "starcanopy: %s: could not write\n", path.c_str());
    return 1;
  }
  std::printf("wrote %s\n", path.c_str());
  return 0;
}

int render(int argc, char** argv) {
  using namespace starcanopy;
  if (argc < 3) {
    std::fputs(kUsage, stderr);
    return 2;
  }
  Project project;
  std::string error;
  if (!loadProject(argv[2], project, error)) {
    std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
    return 1;
  }
  ContextKind kind = ContextKind::Auto;
  for (int i = 3; i < argc; i++) {
    std::string arg = argv[i];
    bool hasValue = i + 1 < argc;
    if (arg == "--macro" && hasValue) {
      std::string assignment = argv[++i];
      size_t eq = assignment.find('=');
      if (eq == std::string::npos ||
          !setMacro(project.macros, assignment.substr(0, eq), assignment.substr(eq + 1), error)) {
        std::fprintf(stderr, "starcanopy: %s\n",
                     eq == std::string::npos ? "--macro wants NAME=VALUE" : error.c_str());
        return 2;
      }
    } else if (arg == "--set" && hasValue) {
      std::string assignment = argv[++i];
      size_t eq = assignment.find('=');
      if (eq == std::string::npos ||
          !setDial(project.settings, assignment.substr(0, eq), assignment.substr(eq + 1), error)) {
        std::fprintf(stderr, "starcanopy: %s\n",
                     eq == std::string::npos ? "--set wants NAME=VALUE" : error.c_str());
        return 2;
      }
    } else if (arg == "--size" && hasValue) {
      project.size = std::atoi(argv[++i]);
      if (project.size < 1 || project.size > 16384) {
        std::fprintf(stderr, "starcanopy: --size must be 1 to 16384\n");
        return 2;
      }
    } else if (arg == "--out" && hasValue) {
      project.output.directory = argv[++i];
    } else if (arg == "--context" && hasValue) {
      if (!parseContextKind(argv[++i], kind)) {
        std::fprintf(stderr, "starcanopy: unknown context '%s'\n", argv[i]);
        return 2;
      }
    } else {
      std::fputs(kUsage, stderr);
      return 2;
    }
  }

  auto context = GlContext::create(kind, error);
  if (!context) {
    std::fprintf(stderr, "starcanopy: no OpenGL 3.3 context: %s\n", error.c_str());
    return 1;
  }
  std::printf("context: %s\n", context->description().c_str());
  auto start = std::chrono::steady_clock::now();
  std::vector<std::string> written;
  if (!renderProject(project, written, error)) {
    std::fprintf(stderr, "starcanopy: %s\n", error.c_str());
    return 1;
  }
  double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::printf("rendered seed %u, look %d, 6 x %d x %d, in %.2f s\n", project.settings.seed,
              project.look, project.size, project.size, seconds);
  for (const std::string& path : written) {
    std::printf("wrote %s\n", path.c_str());
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string command = argc >= 2 ? argv[1] : "";
  if (command == "render") {
    return render(argc, argv);
  }
  if (command == "new") {
    return newProject(argc, argv);
  }
  if (command == "macros") {
    return listMacros();
  }
  if (command == "dials") {
    return listDials();
  }
  bool help = command == "--help" || command == "-h";
  std::fputs(kUsage, help ? stdout : stderr);
  return help ? 0 : 2;
}
