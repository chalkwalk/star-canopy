// The vantage atlas (ROADMAP.md, The galaxy from any star): the galaxy and its
// stars, the nebula off, from fixed places in the disc and out of it, over
// several seeds -- to see what the model makes from each, and where it breaks,
// and then as the bench every step of the galaxy work is judged on.
//
//   atlas --out DIR [--size N] [--seeds 1,7,12] [--vantages a,b] [--set NAME=VALUE]...
//         [--context K] [--measure 1] [--macro NAME=VALUE]... [--toward accents]
//         [--game 1]
//
// With --measure 1, no sheets: for each vantage and seed, the galaxy's glow's
// brightness percentiles over the sky, as its exposure measures them.
// With --toward accents, the first two views look at the sky's two largest
// accents -- open clusters and the rest -- instead of the centre and away.
// With --game 1, beside each sheet the first view as a game shows it, 1920 by
// 1080 and 75 degrees across (DIR/VANTAGE-seedN-game.png); bake at --size 2048
// for a game's skybox.
//
// One sheet a vantage and seed, DIR/VANTAGE-seedN.png: the whole sky above, in
// the Equal Earth projection and galactic coordinates -- the galaxy's centre in the middle, its plane across,
// its north up, so every sheet reads the same way -- and below it four views
// 45 degrees across, north up: toward the centre, away from it, along the
// plane, and toward the pole on the disc's side -- down onto it from above. Output belongs outside the repository, as every
// render does.

#include "accents.h"
#include "bake.h"
#include "cubemap_target.h"
#include "galaxy.h"
#include "gl_context.h"
#include "macros.h"
#include "sample.h"
#include "settings.h"
#include "sky.h"
#include "scene.h"
#include "stars.h"
#include "writers.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

using namespace starcanopy;

namespace {

// Where the observer is: a radius in disc scale lengths (the disc ends at 5.5)
// and a height above the midplane in kpc (the old disc's scale height is 0.3).
struct Vantage {
  const char* name;
  float radius, height;
};

const Vantage kVantages[] = {
  {"centre", 0.5f, 0.03f},       // near the centre, in the bulge
  {"mid-disc", 2.5f, 0.03f},     // inside the Sun's radius
  {"sun", 3.0f, 0.03f},          // the Sun's place
  {"rim", 5.0f, 0.03f},          // near the edge, the light all one way
  {"past-edge", 6.0f, 0.03f},    // just beyond it
  {"above-low", 3.0f, 0.5f},     // out of the disc, a little
  {"above-high", 3.0f, 3.0f},    // well above it, looking down on it
  {"outlier", 6.5f, 1.5f},       // the margin a rare seed may reach
  // Outside it, over the centre: the galaxy whole in the view down, as
  // photographs show one -- to judge its type and its structure by. Beyond the
  // height dial's range, so no sky has it.
  {"portrait", 0.0f, 35.0f},
  // The seed's own place, along its path as --macro galactic puts it.
  {"own", std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()},
};

std::vector<std::string> split(const std::string& list) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= list.size()) {
    size_t comma = list.find(',', start);
    if (comma == std::string::npos) {
      comma = list.size();
    }
    if (comma > start) {
      out.push_back(list.substr(start, comma - start));
    }
    start = comma + 1;
  }
  return out;
}

void normalise(float v[3]) {
  float n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  for (int i = 0; i < 3; i++) {
    v[i] /= n;
  }
}

// A galaxy-frame vector into the sky frame: the transpose of its rotation.
void toSky(const Galaxy& g, const float v[3], float out[3]) {
  for (int k = 0; k < 3; k++) {
    out[k] = g.rot[0 + k] * v[0] + g.rot[3 + k] * v[1] + g.rot[6 + k] * v[2];
  }
}

// The rotation that turns the sky into galactic coordinates: the galaxy's
// centre, as the observer sees it, to +z; its north pole to +y. Rows are those
// axes in the sky frame. From the centre itself, where the centre has no
// direction, any direction in the plane will do.
void galactic(const Galaxy& g, float r[9], bool portrait = false) {
  float toCentre[3] = {-g.observer[0], -g.observer[1], 0.0f}, north[3] = {0.0f, 0.0f, 1.0f};
  // From the portrait the galaxy lies at the pole, which the whole-sky map
  // draws as its bottom edge: there, the map is turned to put it in the middle.
  if (portrait) {
    toCentre[0] = 0.0f;
    toCentre[2] = g.observer[2] > 0.0f ? -1.0f : 1.0f;
    north[0] = 0.0f;
    north[1] = 1.0f;
    north[2] = 0.0f;
  }
  if (!portrait && toCentre[0] * toCentre[0] + toCentre[1] * toCentre[1] < 1e-6f) {
    toCentre[0] = 1.0f;
  }
  normalise(toCentre);
  float z[3], y[3], x[3];
  toSky(g, toCentre, z);
  toSky(g, north, y);
  // x = y cross z, so that +x is to the right of +z with +y up.
  x[0] = y[1] * z[2] - y[2] * z[1];
  x[1] = y[2] * z[0] - y[0] * z[2];
  x[2] = y[0] * z[1] - y[1] * z[0];
  for (int k = 0; k < 3; k++) {
    r[0 + k] = x[k];
    r[3 + k] = y[k];
    r[6 + k] = z[k];
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string outDir, error;
  int size = 1024;
  std::vector<uint32_t> seeds = {1, 7, 12};
  std::vector<std::string> names, sets;
  ContextKind kind = ContextKind::Auto;
  bool measure = false, towardAccents = false, game = false;
  MacroValues macroValues;
  for (int i = 1; i + 1 < argc; i += 2) {
    std::string arg = argv[i], v = argv[i + 1];
    if (arg == "--out") {
      outDir = v;
    } else if (arg == "--size") {
      size = std::atoi(v.c_str());
    } else if (arg == "--seeds") {
      seeds.clear();
      for (const std::string& s : split(v)) {
        seeds.push_back(static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10)));
      }
    } else if (arg == "--vantages") {
      names = split(v);
    } else if (arg == "--set") {
      sets.push_back(v);
    } else if (arg == "--macro") {
      size_t eq = v.find('=');
      if (eq == std::string::npos || !setMacro(macroValues, v.substr(0, eq), v.substr(eq + 1), error)) {
        std::fprintf(stderr, "atlas: %s\n", eq == std::string::npos ? "--macro wants NAME=VALUE"
                                                                      : error.c_str());
        return 2;
      }
    } else if (arg == "--toward") {
      if (v != "accents") {
        std::fprintf(stderr, "atlas: --toward takes accents\n");
        return 2;
      }
      towardAccents = true;
    } else if (arg == "--game") {
      game = v == "1";
    } else if (arg == "--measure") {
      measure = v == "1";
    } else if (arg == "--context") {
      if (!parseContextKind(v, kind)) {
        std::fprintf(stderr, "atlas: unknown context %s\n", v.c_str());
        return 2;
      }
    } else {
      std::fprintf(stderr, "atlas: unknown option %s\n", arg.c_str());
      return 2;
    }
  }
  if (outDir.empty() || size < 16) {
    std::fprintf(stderr, "usage: atlas --out DIR [--size N] [--seeds 1,7,12] [--vantages a,b] "
                         "[--set NAME=VALUE]... [--measure 1]\n");
    return 2;
  }
  std::vector<const Vantage*> list;
  for (const Vantage& v : kVantages) {
    bool wanted = names.empty();
    for (const std::string& n : names) {
      wanted = wanted || n == v.name;
    }
    if (wanted) {
      list.push_back(&v);
    }
  }
  if (list.empty()) {
    std::fprintf(stderr, "atlas: no such vantages\n");
    return 2;
  }

  auto context = GlContext::create(kind, error);
  if (!context) {
    std::fprintf(stderr, "atlas: no context: %s\n", error.c_str());
    return 1;
  }
  Baker baker;
  if (!baker.ok(error)) {
    std::fprintf(stderr, "atlas: %s\n", error.c_str());
    return 1;
  }
  std::filesystem::create_directories(outDir);

  const int wide = 2048, viewSize = 512, gap = 16;
  size_t total = list.size() * seeds.size(), done = 0;
  for (const Vantage* v : list) {
    for (uint32_t seed : seeds) {
      Settings s;
      s.seed = seed;
      s.nebula = 0;
      s.galaxyRadius = v->radius;
      s.galaxyHeight = v->height;
      // Raw dials over every sheet's, for trying a step before it is made.
      for (const std::string& assignment : sets) {
        size_t eq = assignment.find('=');
        if (eq == std::string::npos ||
            !setDial(s, assignment.substr(0, eq), assignment.substr(eq + 1), error)) {
          std::fprintf(stderr, "atlas: %s\n", eq == std::string::npos ? "--set wants NAME=VALUE"
                                                                        : error.c_str());
          return 2;
        }
      }
      s = resolveMacros(s, macroValues);
      // The glow's brightness from here, as the exposure measures it, instead
      // of a sheet: for setting the exposure's reference (sky.cpp).
      if (measure) {
        Sky sk = buildSky(s);
        Scene sc = generateScene(sk.scene);
        Galaxy gx = generateGalaxy(seed, sk.galaxy);
        float band[3] = {0.0f, 0.0f, sk.galaxyHaze / gx.grain};
        generateStars(sc, gx, sk.stars, &band[0]);
        band[1] = 0.49f * sk.stars.reach * sk.stars.reach;
        float m[4];
        baker.measureGalaxy(gx, sk.look.reddening, band, m);
        std::printf("%-11s seed %2u  p50 %.4f  p90 %.4f  p99 %.4f  p99.9 %.4f\n", v->name, seed,
                    m[0], m[1], m[2], m[3]);
        continue;
      }
      CubemapTarget target(size);
      bakeSky(baker, s, target);
      Sky sky = buildSky(s);
      Galaxy g = generateGalaxy(seed, sky.galaxy);
      float r[9];
      galactic(g, r);
      Cubemap turned = rotate(target.read(), r);

      bool portrait = v->height > 10.0f;
      Image whole;
      if (portrait) {
        float rp[9];
        galactic(g, rp, true);
        whole = equalEarth(rotate(target.read(), rp), wide);
      } else {
        whole = equalEarth(turned, wide);
      }
      Image sheet;
      sheet.width = wide;
      sheet.height = whole.height + gap + viewSize;
      sheet.rgb.assign(static_cast<size_t>(sheet.width) * sheet.height * 3, 0.02f);
      paste(sheet, whole, 0, 0);
      // In galactic coordinates: the centre +z, north +y, the plane's other
      // way +x.
      float toDisc = g.observer[2] > 0.0f ? -1.0f : 1.0f;
      float views[4][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {0, toDisc, 0}};
      // Or the first two toward the sky's two largest accents, to judge them by.
      if (towardAccents) {
        std::vector<Accent> accents = generateAccents(seed, g, sky.accents);
        std::sort(accents.begin(), accents.end(), [](const Accent& a, const Accent& b) {
          return a.radius / a.distance > b.radius / b.distance;
        });
        for (size_t k = 0; k < 2 && k < accents.size(); k++) {
          for (int i = 0; i < 3; i++) {
            views[k][i] = r[3 * i + 0] * accents[k].dir[0] + r[3 * i + 1] * accents[k].dir[1] +
                          r[3 * i + 2] * accents[k].dir[2];
          }
        }
      }
      const float north[3] = {0, 1, 0}, plane[3] = {0, 0, 1};
      for (int k = 0; k < 4; k++) {
        paste(sheet, perspective(turned, views[k], k == 3 ? plane : north, 45.0f, viewSize, viewSize),
              k * viewSize, whole.height + gap);
      }
      // Built as strings: a fixed buffer cut a long output path's name short.
      std::string stem = outDir + "/" + v->name + "-seed" + std::to_string(seed);
      // And, beside it, the first view as a game shows it: 1920 by 1080, 75
      // degrees across -- the sheet's views are half a game's resolution.
      if (game) {
        if (!writePng(stem + "-game.png", perspective(turned, views[0], north, 75.0f, 1920, 1080),
                      error)) {
          std::fprintf(stderr, "atlas: %s\n", error.c_str());
          return 1;
        }
      }
      if (!writePng(stem + ".png", sheet, error)) {
        std::fprintf(stderr, "atlas: %s\n", error.c_str());
        return 1;
      }
      std::fprintf(stderr, "\ratlas %zu/%zu", ++done, total);
    }
  }
  std::fprintf(stderr, "\natlas: %zu sheets in %s\n", total, outDir.c_str());
  return 0;
}
