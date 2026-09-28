#include "project.h"

// Errors are returned, not thrown, as everywhere else here.
#define TOML_EXCEPTIONS 0
#include "toml.hpp"

#include <cstdio>
#include <filesystem>
#include <set>

namespace starcanopy {

namespace {

bool onlyKeys(const toml::table& t, const std::set<std::string>& known, const std::string& where,
              std::string& error) {
  for (const auto& [key, value] : t) {
    if (!known.count(std::string(key.str()))) {
      error = "unknown key '" + std::string(key.str()) + "'" + (where.empty() ? "" : " in [" + where + "]");
      return false;
    }
  }
  return true;
}

// A value as the text setDial() reads.
bool valueText(const toml::node& v, std::string& text) {
  if (auto i = v.as_integer()) {
    text = std::to_string(i->get());
  } else if (auto f = v.as_floating_point()) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.9g", f->get());
    text = buf;
  } else if (auto s = v.as_string()) {
    text = s->get();
  } else {
    return false;
  }
  return true;
}

bool number(const toml::table& t, const char* key, float& out, std::string& error) {
  if (const toml::node* n = t.get(key)) {
    auto v = n->value<double>();
    if (!v) {
      error = std::string("'") + key + "' should be a number";
      return false;
    }
    out = static_cast<float>(*v);
  }
  return true;
}

}  // namespace

bool loadProject(const std::string& path, Project& p, std::string& error) {
  toml::parse_result parsed = toml::parse_file(path);
  if (!parsed) {
    const auto& e = parsed.error();
    error = path + ":" + std::to_string(e.source().begin.line) + ": " + std::string(e.description());
    return false;
  }
  const toml::table& t = parsed.table();
  auto fail = [&](const std::string& message) {
    error = path + ": " + message;
    return false;
  };
  if (!onlyKeys(t, {"look", "seed", "style", "macros", "overrides", "orientation", "output"}, "",
                error)) {
    return fail(error);
  }

  auto look = t["look"].value<int64_t>();
  if (!look) {
    return fail("no look version: a project says which look it was made with, as look = " +
                std::to_string(kLookVersion));
  }
  if (*look > kLookVersion) {
    return fail("made with look " + std::to_string(*look) + ", which is newer than this StarCanopy's (" +
                std::to_string(kLookVersion) + "); a newer StarCanopy renders it");
  }
  if (*look < 1) {
    return fail("look " + std::to_string(*look) + " is not a look version");
  }
  if (*look < kLookVersion) {
    return fail("made with look " + std::to_string(*look) +
                ", which this StarCanopy no longer renders (see project.h for what each look "
                "changed); to render it with look " + std::to_string(kLookVersion) +
                ", set look = " + std::to_string(kLookVersion));
  }
  p.look = static_cast<int>(*look);

  if (const toml::node* seed = t.get("seed")) {
    std::string text;
    if (!valueText(*seed, text) || !setDial(p.settings, "seed", text, error)) {
      return fail(error.empty() ? "'seed' should be a whole number" : error);
    }
  }
  if (const toml::node* style = t.get("style")) {
    auto s = style->value<std::string>();
    if (!s || *s != "mass") {
      return fail(s && *s == "shell" ? "the shell style was retired in look 2; style is \"mass\""
                                     : "style is \"mass\"");
    }
  }

  if (const toml::node* macros = t.get("macros")) {
    const toml::table* m = macros->as_table();
    if (!m) {
      return fail("[macros] should be a table");
    }
    for (const auto& [key, value] : *m) {
      std::string name(key.str()), text;
      if (!valueText(value, text) || !value.is_number()) {
        return fail("[macros] " + name + " should be a number, -1 to 1");
      }
      if (!setMacro(p.macros, name, text, error)) {
        return fail("[macros] " + error + "; raw dials go in [overrides]");
      }
    }
  }

  if (const toml::node* overrides = t.get("overrides")) {
    const toml::table* o = overrides->as_table();
    if (!o) {
      return fail("[overrides] should be a table");
    }
    for (const auto& [key, value] : *o) {
      std::string name(key.str()), text;
      if (name == "seed") {
        return fail("'seed' is set at the top of the project, not in [overrides]");
      }
      if (!valueText(value, text)) {
        return fail("[overrides] " + name + " should be a number or a name");
      }
      if (!setDial(p.settings, name, text, error)) {
        return fail("[overrides] " + error);
      }
    }
  }

  if (const toml::node* orientation = t.get("orientation")) {
    const toml::table* o = orientation->as_table();
    if (!o || !onlyKeys(*o, {"yaw", "pitch", "roll"}, "orientation", error) ||
        !number(*o, "yaw", p.yaw, error) || !number(*o, "pitch", p.pitch, error) ||
        !number(*o, "roll", p.roll, error)) {
      return fail(o ? error : "[orientation] should be a table");
    }
  }

  // Outputs go beside the project file unless it says otherwise.
  std::filesystem::path base = std::filesystem::path(path).parent_path();
  p.output.directory = base.empty() ? "." : base.string();
  if (const toml::node* output = t.get("output")) {
    const toml::table* o = output->as_table();
    if (!o) {
      return fail("[output] should be a table");
    }
    if (!onlyKeys(*o, {"size", "directory", "name", "formats", "equirect_width"}, "output", error)) {
      return fail(error);
    }
    if (const toml::node* size = o->get("size")) {
      auto v = size->value<int64_t>();
      if (!v || *v < 1 || *v > 16384) {
        return fail("[output] size is a whole number of texels, 1 to 16384");
      }
      p.size = static_cast<int>(*v);
    }
    if (const toml::node* dir = o->get("directory")) {
      auto v = dir->value<std::string>();
      if (!v) {
        return fail("[output] directory should be a string");
      }
      std::filesystem::path d(*v);
      p.output.directory = d.is_absolute() || base.empty() ? d.string() : (base / d).string();
    }
    if (const toml::node* name = o->get("name")) {
      auto v = name->value<std::string>();
      if (!v || v->empty() || v->find('/') != std::string::npos) {
        return fail("[output] name should be a file name, without a directory");
      }
      p.output.name = *v;
    }
    if (const toml::node* formats = o->get("formats")) {
      const toml::array* a = formats->as_array();
      if (!a) {
        return fail("[output] formats should be a list, such as [\"exr-faces\", \"ktx2\"]");
      }
      p.output.formats.clear();
      for (const toml::node& f : *a) {
        auto v = f.value<std::string>();
        bool known = false;
        for (const std::string& k : outputFormats()) {
          known = known || (v && *v == k);
        }
        if (!known) {
          std::string list;
          for (const std::string& k : outputFormats()) {
            list += " " + k;
          }
          return fail("[output] formats: unknown format; the formats are" + list);
        }
        p.output.formats.push_back(*v);
      }
    }
    if (const toml::node* width = o->get("equirect_width")) {
      auto v = width->value<int64_t>();
      if (!v || *v < 0 || *v > 32768) {
        return fail("[output] equirect_width is a whole number, 0 to 32768");
      }
      p.output.equirectWidth = static_cast<int>(*v);
    }
  }
  return true;
}

std::string projectText(uint32_t seed, const std::string& name) {
  char head[512];
  std::snprintf(head, sizeof(head),
                "# A StarCanopy project: how a sky is made again.\n"
                "\n"
                "look = %d        # the look version it was made with; do not change it by hand\n"
                "seed = %u\n"
                "style = \"mass\"\n"
                "\n"
                "[macros]        # -1..1; 0 is the seed's own sky\n",
                kLookVersion, seed);
  std::string text = head;
  // Every macro, at 0, with its two ends: the controls, where they are seen.
  int count = 0;
  const Macro* m = macros(count);
  for (int i = 0; i < count; i++) {
    char line[256];
    std::snprintf(line, sizeof(line), "%-12s = 0.0   # -1 %s .. 1 %s: %s\n", m[i].name,
                  m[i].opposite, m[i].name, m[i].help);
    text += line;
  }
  char tail[768];
  std::snprintf(tail, sizeof(tail),
                "\n"
                "[overrides]     # raw dials, for scripting: `starcanopy dials` lists them\n"
                "\n"
                "[orientation]   # degrees: yaw about +y, pitch about +x, roll about +z\n"
                "yaw = 0.0\n"
                "pitch = 0.0\n"
                "roll = 0.0\n"
                "\n"
                "[output]\n"
                "size = 2048     # texels per face\n"
                "directory = \"%s\"\n"
                "name = \"%s\"\n"
                "# exr-faces exr-equirect ktx2 png-faces png-cross png-equirect\n"
                "formats = [\"exr-faces\", \"ktx2\", \"png-faces\"]\n",
                name.c_str(), name.c_str());
  return text + tail;
}

}  // namespace starcanopy
