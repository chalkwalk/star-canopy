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
  CHECK(load(projectText(42, "shell", "night"), p, error));
  CHECK(p.look == kLookVersion && p.settings.seed == 42 && p.settings.form == 0);
  CHECK(p.size == 2048 && p.output.name == "night" && p.output.directory == dir + "/night");
  CHECK(p.output.formats.size() == 3);

  // Everything a project can say.
  CHECK(load("look = 1\nseed = 7\nstyle = \"mass\"\n[macros]\n"
             "[overrides]\ndensity = 1.5\nexposure = 0.25\nline-colors = \"hoo\"\nclouds = 2\n"
             "[orientation]\nyaw = 90\npitch = -10.5\n"
             "[output]\nsize = 512\ndirectory = \"/tmp/x\"\nname = \"a\"\n"
             "formats = [\"ktx2\", \"png-cross\"]\nequirect_width = 1000\n",
             p, error));
  if (!error.empty()) {
    std::printf("%s\n", error.c_str());
  }
  CHECK(p.settings.seed == 7 && p.settings.form == 1);
  CHECK(p.settings.density == 1.5f && p.settings.exposure == 0.25f);
  CHECK(p.settings.lineColors == 2 && p.settings.clouds == 2);
  CHECK(p.yaw == 90.0f && p.pitch == -10.5f && p.roll == 0.0f);
  CHECK(p.size == 512 && p.output.directory == "/tmp/x" && p.output.name == "a");
  CHECK(p.output.formats.size() == 2 && p.output.equirectWidth == 1000);

  // The look version is required, and a newer one refused.
  CHECK(fails("seed = 1\n", "look"));
  CHECK(fails("look = 2\n", "newer"));
  CHECK(fails("look = 0\n", "not a look version"));
  // Nothing unknown passes silently.
  CHECK(fails("look = 1\nsed = 3\n", "sed"));
  CHECK(fails("look = 1\n[overrides]\ndensty = 1\n", "densty"));
  CHECK(fails("look = 1\n[output]\nsise = 1\n", "sise"));
  CHECK(fails("look = 1\n[orientation]\nturn = 1\n", "turn"));
  CHECK(fails("look = 1\n[output]\nformats = [\"gif\"]\n", "formats"));
  // Nor anything malformed or out of range.
  CHECK(fails("look = 1\nstyle = \"cloud\"\n", "style"));
  CHECK(fails("look = 1\n[overrides]\ndensity = 1000\n", "density"));
  CHECK(fails("look = 1\n[overrides]\nseed = 3\n", "seed"));
  CHECK(fails("look = 1\n[overrides]\nform = \"mass\"\n", "style"));
  CHECK(fails("look = 1\n[macros]\nopen = 0.5\n", "macros"));
  CHECK(fails("look = 1\n[output]\nsize = 0\n", "size"));
  CHECK(fails("look = 1\n[output]\nname = \"a/b\"\n", "name"));
  CHECK(fails("look = 1\nseed = \n", "project.toml:2"));  // a TOML error, with its line
  return test::finish();
}
