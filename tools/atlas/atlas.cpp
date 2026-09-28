// The vantage atlas (ROADMAP.md, The galaxy from any star): the galaxy and its
// stars, the nebula off, from fixed places in the disc and out of it, over
// several seeds -- to see what the model makes from each, and where it breaks,
// and then as the bench every step of the galaxy work is judged on.
//
//   atlas --out DIR [--size N] [--seeds 1,7,12] [--vantages a,b] [--context K]
//
// One sheet a vantage and seed, DIR/VANTAGE-seedN.png: the whole sky above, in
// galactic coordinates -- the galaxy's centre in the middle, its plane across,
// its north up, so every sheet reads the same way -- and below it four views
// 45 degrees across, north up: toward the centre, away from it, along the
// plane, and toward the pole on the disc's side -- down onto it from above. Output belongs outside the repository, as every
// render does.

#include "bake.h"
#include "cubemap_target.h"
#include "galaxy.h"
#include "gl_context.h"
#include "sample.h"
#include "settings.h"
#include "sky.h"
#include "writers.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
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
void galactic(const Galaxy& g, float r[9]) {
  float toCentre[3] = {-g.observer[0], -g.observer[1], 0.0f}, north[3] = {0.0f, 0.0f, 1.0f};
  if (toCentre[0] * toCentre[0] + toCentre[1] * toCentre[1] < 1e-6f) {
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
  std::vector<std::string> names;
  ContextKind kind = ContextKind::Auto;
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
    std::fprintf(stderr, "usage: atlas --out DIR [--size N] [--seeds 1,7,12] [--vantages a,b]\n");
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
      CubemapTarget target(size);
      bakeSky(baker, s, target);
      Sky sky = buildSky(s);
      Galaxy g = generateGalaxy(seed, sky.galaxy);
      float r[9];
      galactic(g, r);
      Cubemap turned = rotate(target.read(), r);

      Image sheet;
      sheet.width = wide;
      sheet.height = wide / 2 + gap + viewSize;
      sheet.rgb.assign(static_cast<size_t>(sheet.width) * sheet.height * 3, 0.02f);
      paste(sheet, equirect(turned, wide), 0, 0);
      // In galactic coordinates: the centre +z, north +y, the plane's other
      // way +x.
      float toDisc = v->height > 0.0f ? -1.0f : 1.0f;
      const float views[4][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {0, toDisc, 0}};
      const float north[3] = {0, 1, 0}, plane[3] = {0, 0, 1};
      for (int k = 0; k < 4; k++) {
        paste(sheet, perspective(turned, views[k], k == 3 ? plane : north, 45.0f, viewSize, viewSize),
              k * viewSize, wide / 2 + gap);
      }
      char name[128];
      std::snprintf(name, sizeof(name), "%s/%s-seed%u.png", outDir.c_str(), v->name, seed);
      if (!writePng(name, sheet, error)) {
        std::fprintf(stderr, "atlas: %s\n", error.c_str());
        return 1;
      }
      std::fprintf(stderr, "\ratlas %zu/%zu", ++done, total);
    }
  }
  std::fprintf(stderr, "\natlas: %zu sheets in %s\n", total, outDir.c_str());
  return 0;
}
