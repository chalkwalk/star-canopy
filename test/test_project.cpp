// Project files: what they say is what is set; a look version is required, and
// a newer one refused; anything unknown or malformed is an error with a
// reason, never silently ignored.

#include "check.h"
#include "project.h"

#include <cstdio>
#include <fstream>
#include <string>

using namespace starcanopy;

namespace {

std::string dir;

bool load(const std::string& text, Project& p, std::string& error) {
  std::string path = dir + "/project.toml";
  std::ofstream(path) << text;
  p = Project{};
  error.clear();
  return loadProject(path, p, error);
}

bool fails(const std::string& text, const char* expect) {
  Project p;
  std::string error;
  if (load(text, p, error)) {
    std::printf("loaded, but should not have: %s\n", text.c_str());
    return false;
  }
  if (error.find(expect) == std::string::npos) {
    std::printf("error '%s' does not mention '%s'\n", error.c_str(), expect);
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: test_project SCRATCH_DIR\n");
    return 2;
  }
  dir = argv[1];
  Project p;
  std::string error;

  // A new project loads, as written.
  CHECK(load(projectText(42, "night"), p, error));
  CHECK(p.look == kLookVersion && kLookVersion == 4 && p.settings.seed == 42);
  CHECK(p.size == 2048 && p.output.name == "night" && p.output.directory == dir + "/night");
  CHECK(p.output.formats.size() == 3);
  // It lists every macro, at 0: the seed's own sky.
  {
    int count = 0;
    const Macro* m = macros(count);
    std::string text = projectText(42, "night");
    for (int i = 0; i < count; i++) {
      CHECK(text.find(std::string("\n") + m[i].name + " ") != std::string::npos);
    }
    CHECK(p.macros.size() == static_cast<size_t>(count));
    for (const auto& [name, value] : p.macros) {
      CHECK(value == 0.0f);
    }
  }

  // Everything a project can say.
  CHECK(load("look = 4\nseed = 7\nstyle = \"mass\"\n[macros]\nopen = 0.5\nbright = -1\n"
             "[overrides]\ndensity = 1.5\nexposure = 0.25\nline-colors = \"hoo\"\nmass-clusters = 2\n"
             "[orientation]\nyaw = 90\npitch = -10.5\n"
             "[output]\nsize = 512\ndirectory = \"/tmp/x\"\nname = \"a\"\n"
             "formats = [\"ktx2\", \"png-cross\"]\nequirect_width = 1000\n",
             p, error));
  if (!error.empty()) {
    std::printf("%s\n", error.c_str());
  }
  CHECK(p.settings.seed == 7);
  CHECK(p.settings.density == 1.5f && p.settings.exposure == 0.25f);
  CHECK(p.settings.lineColors == 2 && p.settings.massClusters == 2);
  // Macros are kept apart from the overrides, which are their base.
  CHECK(p.macros.size() == 2 && p.macros["open"] == 0.5f && p.macros["bright"] == -1.0f);
  CHECK(p.resolved().exposure == 0.125f && p.resolved().density == 1.5f);
  CHECK(p.yaw == 90.0f && p.pitch == -10.5f && p.roll == 0.0f);
  CHECK(p.size == 512 && p.output.directory == "/tmp/x" && p.output.name == "a");
  CHECK(p.output.formats.size() == 2 && p.output.equirectWidth == 1000);

  // The look version is required, and a newer one refused.
  CHECK(fails("seed = 1\n", "look"));
  CHECK(fails("look = 5\n", "newer"));
  // Look 1 is refused, saying why and what to do.
  CHECK(fails("look = 1\n", "look = 4"));
  CHECK(fails("look = 3\n", "look = 4"));
  CHECK(fails("look = 0\n", "not a look version"));
  // Nothing unknown passes silently.
  CHECK(fails("look = 4\nsed = 3\n", "sed"));
  CHECK(fails("look = 4\n[overrides]\ndensty = 1\n", "densty"));
  CHECK(fails("look = 4\n[output]\nsise = 1\n", "sise"));
  CHECK(fails("look = 4\n[orientation]\nturn = 1\n", "turn"));
  CHECK(fails("look = 4\n[output]\nformats = [\"gif\"]\n", "formats"));
  // Nor anything malformed or out of range.
  CHECK(fails("look = 4\nstyle = \"cloud\"\n", "style"));
  CHECK(fails("look = 4\n[overrides]\ndensity = 1000\n", "density"));
  CHECK(fails("look = 4\n[overrides]\nseed = 3\n", "seed"));
  CHECK(fails("look = 4\nstyle = \"shell\"\n", "retired"));
  CHECK(fails("look = 4\n[overrides]\nform = \"mass\"\n", "form"));
  CHECK(fails("look = 4\n[macros]\nopne = 0.5\n", "opne"));
  CHECK(fails("look = 4\n[macros]\nopen = 1.5\n", "open"));
  CHECK(fails("look = 4\n[macros]\nopen = \"wide\"\n", "open"));
  CHECK(fails("look = 4\n[macros]\ndensity = 1\n", "[overrides]"));
  CHECK(fails("look = 4\n[output]\nsize = 0\n", "size"));
  CHECK(fails("look = 4\n[output]\nname = \"a/b\"\n", "name"));
  CHECK(fails("look = 4\nseed = \n", "project.toml:2"));  // a TOML error, with its line
  return test::finish();
}
