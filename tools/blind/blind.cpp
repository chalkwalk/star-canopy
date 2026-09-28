// Blind pairs for the macros (PRINCIPLES §1, §2): does each macro move the sky
// the way its name says, judged by eye at a game's field of view, not by a
// number or a whole-sky map.
//
//   blind --out DIR [--size N] [--seeds 3,7,12] [--macros a,b] [--value V]
//         [--around] [--three] [--context K]
//
// For each macro and seed, the sky at -V and at +V (default 1), each seen
// twice at 75 degrees across, 16:9: toward the key light, and turned 150
// degrees from it. With --around, six times, smaller, every 60 degrees round
// the horizon from the key light: for what shows only as one looks around,
// such as the size of the forms. With --three, each sheet is three skies, at
// -V, 0 and +V in an order drawn at random, labelled A, B, C from the left, to
// be put in order: the macro's two ends seen against the seed's own sky. One sheet a pair, the two skies side by side, which side is
// which drawn at random; the pairs shuffled. The key goes to DIR/key.csv, to be
// opened only when the scores are in; DIR/scores.md is the sheet to score on.
//
// Output belongs outside the repository: sheets are pictures of skies, and
// judging them is the human's (AGENTS.md).

#include "bake.h"
#include "cubemap_target.h"
#include "gl_context.h"
#include "macros.h"
#include "sample.h"
#include "settings.h"
#include "sky.h"
#include "writers.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace starcanopy;

namespace {

int kViewWidth = 800, kViewHeight = 450;
constexpr int kGap = 16;
constexpr float kFovDegrees = 75.0f;

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

void crossProduct(const float a[3], const float b[3], float out[3]) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

// A pinhole view of the sky along `forward`, level with the sky's +y, drawn
// into `sheet` at (x0, y0).
void view(const Cubemap& sky, const float forwardIn[3], Image& sheet, int x0, int y0) {
  float forward[3] = {forwardIn[0], forwardIn[1], forwardIn[2]}, up[3] = {0, 1, 0}, right[3];
  normalise(forward);
  if (std::fabs(forward[1]) > 0.95f) {
    up[1] = 0.0f;
    up[2] = 1.0f;
  }
  crossProduct(forward, up, right);
  normalise(right);
  crossProduct(right, forward, up);
  float half = std::tan(kFovDegrees * 0.5f * 3.14159265f / 180.0f);
  for (int y = 0; y < kViewHeight; y++) {
    for (int x = 0; x < kViewWidth; x++) {
      float sx = ((x + 0.5f) / kViewWidth * 2.0f - 1.0f) * half;
      float sy = (1.0f - (y + 0.5f) / kViewHeight * 2.0f) * half * kViewHeight / kViewWidth;
      float d[3], c[3];
      for (int i = 0; i < 3; i++) {
        d[i] = forward[i] + sx * right[i] + sy * up[i];
      }
      sampleCube(sky, d, c);
      float* p = &sheet.rgb[3 * ((y0 + y) * sheet.width + x0 + x)];
      p[0] = c[0];
      p[1] = c[1];
      p[2] = c[2];
    }
  }
}

// The key light turned `degrees` about +y, level; +z if it is overhead.
void turned(const float light[3], float degrees, float out[3]) {
  float a = degrees * 3.14159265f / 180.0f;
  out[0] = std::cos(a) * light[0] + std::sin(a) * light[2];
  out[1] = 0.0f;
  out[2] = -std::sin(a) * light[0] + std::cos(a) * light[2];
  if (std::fabs(out[0]) + std::fabs(out[2]) < 1e-3f) {
    out[2] = 1.0f;
  }
}

// The views of one sky at column x0: toward the key light above, and turned
// 150 degrees from it below; or, around, two columns of three every 60 degrees
// from the key light, level.
void views(const Cubemap& sky, const Settings& s, bool around, Image& sheet, int x0) {
  float light[3], d[3];
  keyLight(s, light);
  if (!around) {
    turned(light, 150.0f, d);
    view(sky, light, sheet, x0, 0);
    view(sky, d, sheet, x0, kViewHeight + kGap);
    return;
  }
  for (int k = 0; k < 6; k++) {
    turned(light, 60.0f * k, d);
    view(sky, d, sheet, x0 + (k % 2) * (kViewWidth + kGap / 2), (k / 2) * (kViewHeight + kGap / 2));
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string outDir, error;
  int size = 1024;
  float value = 1.0f;
  bool around = false, three = false;
  std::vector<uint32_t> seeds = {3, 7, 12};
  std::vector<std::string> names;
  ContextKind kind = ContextKind::Auto;
  for (int i = 1; i < argc; i += 2) {
    std::string arg = argv[i];
    if (arg == "--around" || arg == "--three") {
      (arg == "--around" ? around : three) = true;
      i--;
      continue;
    }
    if (i + 1 >= argc) {
      std::fprintf(stderr, "blind: %s wants a value\n", arg.c_str());
      return 2;
    }
    std::string v = argv[i + 1];
    if (arg == "--out") {
      outDir = v;
    } else if (arg == "--size") {
      size = std::atoi(v.c_str());
    } else if (arg == "--value") {
      value = static_cast<float>(std::atof(v.c_str()));
    } else if (arg == "--seeds") {
      seeds.clear();
      for (const std::string& s : split(v)) {
        seeds.push_back(static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10)));
      }
    } else if (arg == "--macros") {
      names = split(v);
    } else if (arg == "--context") {
      if (!parseContextKind(v, kind)) {
        std::fprintf(stderr, "blind: unknown context %s\n", v.c_str());
        return 2;
      }
    } else {
      std::fprintf(stderr, "blind: unknown option %s\n", arg.c_str());
      return 2;
    }
  }
  if (outDir.empty() || size < 16 || !(value > 0.0f && value <= 1.0f)) {
    std::fprintf(stderr,
                 "usage: blind --out DIR [--size N] [--seeds 3,7,12] [--macros a,b] [--value V]\n"
                 "             [--around] [--three]\n");
    return 2;
  }
  if (around) {
    kViewWidth = three ? 400 : 480;
    kViewHeight = three ? 225 : 270;
  }
  // One side's width and height: one column of two views, or two of three.
  int sideWidth = around ? 2 * kViewWidth + kGap / 2 : kViewWidth;
  int sideHeight = around ? 3 * kViewHeight + kGap : 2 * kViewHeight + kGap;
  int count = 0;
  const Macro* all = macros(count);
  std::vector<const Macro*> list;
  for (int i = 0; i < count; i++) {
    if (names.empty() || std::find(names.begin(), names.end(), all[i].name) != names.end()) {
      list.push_back(&all[i]);
    }
  }
  if (list.empty()) {
    std::fprintf(stderr, "blind: no such macros\n");
    return 2;
  }

  auto context = GlContext::create(kind, error);
  if (!context) {
    std::fprintf(stderr, "blind: no context: %s\n", error.c_str());
    return 1;
  }
  Baker baker;
  if (!baker.ok(error)) {
    std::fprintf(stderr, "blind: %s\n", error.c_str());
    return 1;
  }
  std::filesystem::create_directories(outDir);

  struct Pair {
    const Macro* macro;
    uint32_t seed;
  };
  std::vector<Pair> pairs;
  for (const Macro* m : list) {
    for (uint32_t seed : seeds) {
      pairs.push_back({m, seed});
    }
  }
  // Not reproducible on purpose: a scorer who could rerun the draw could know
  // the key.
  std::mt19937 rng(std::random_device{}());
  std::shuffle(pairs.begin(), pairs.end(), rng);

  std::unique_ptr<FILE, int (*)(FILE*)> key(std::fopen((outDir + "/key.csv").c_str(), "w"), std::fclose);
  std::unique_ptr<FILE, int (*)(FILE*)> scores(std::fopen((outDir + "/scores.md").c_str(), "w"),
                                               std::fclose);
  if (!key || !scores) {
    std::fprintf(stderr, "blind: cannot write in %s\n", outDir.c_str());
    return 1;
  }
  const char* seen = around ? "each seen six times round the horizon from its key light (reading\n"
                              "across, then down)"
                            : "each seen toward its key light (top) and turned away from it (bottom)";
  if (three) {
    std::fprintf(key.get(), "sheet,macro,seed,A,B,C\n");
    std::fprintf(scores.get(),
                 "# Blind sheets: the macros, three ways\n\n"
                 "Each sheet is one seed's sky three times, A, B and C from the left: at one end\n"
                 "of one macro, at its other end, and as the seed made it, in an order drawn at\n"
                 "random -- %s, at 75 degrees across. Do not open key.csv until every line below is\n"
                 "filled in.\n\n"
                 "For each sheet: the three in order from least to most as the question says (such\n"
                 "as B A C), and which you would rather have as a sky (A, B, C, or = for no\n"
                 "preference). A note if any is a sky nobody would want.\n\n"
                 "| sheet | question | least to most | rather have | note |\n"
                 "|---|---|---|---|---|\n",
                 seen);
  } else {
    std::fprintf(key.get(), "pair,macro,seed,left,right\n");
    std::fprintf(scores.get(),
                 "# Blind pairs: the macros\n\n"
                 "Each sheet is one seed's sky at the two ends of one macro, left and right, %s,\n"
                 "at 75 degrees across. Which side is which is drawn at random. Do not open key.csv\n"
                 "until every line below is filled in.\n\n"
                 "For each pair: which side is more as the question says (L or R), and which you\n"
                 "would rather have as a sky (L, R or = for no preference). A note if either end\n"
                 "is a sky nobody would want.\n\n"
                 "| pair | question | more so | rather have | note |\n"
                 "|---|---|---|---|---|\n",
                 seen);
  }
  for (size_t i = 0; i < pairs.size(); i++) {
    const Pair& p = pairs[i];
    std::vector<float> values = {-value, value};
    if (three) {
      values.push_back(0.0f);
    }
    std::shuffle(values.begin(), values.end(), rng);
    int sides = static_cast<int>(values.size());
    Image sheet;
    sheet.width = sides * sideWidth + (sides - 1) * 2 * kGap;
    sheet.height = sideHeight;
    // The gaps a mid grey, so no side's dark bleeds into the next.
    sheet.rgb.assign(static_cast<size_t>(sheet.width) * sheet.height * 3, 0.02f);
    for (int side = 0; side < sides; side++) {
      Settings base;
      base.seed = p.seed;
      Settings s = resolveMacros(base, {{p.macro->name, values[side]}});
      CubemapTarget target(size);
      bakeSky(baker, s, target);
      views(target.read(), s, around, sheet, side * (sideWidth + 2 * kGap));
    }
    char name[32];
    std::snprintf(name, sizeof(name), three ? "sheet%02zu.png" : "pair%02zu.png", i + 1);
    if (!writePng(outDir + "/" + name, sheet, error)) {
      std::fprintf(stderr, "blind: %s\n", error.c_str());
      return 1;
    }
    std::fprintf(key.get(), "%zu,%s,%u", i + 1, p.macro->name, p.seed);
    for (float v : values) {
      std::fprintf(key.get(), ",%g", static_cast<double>(v));
    }
    std::fprintf(key.get(), "\n");
    if (three) {
      std::fprintf(scores.get(), "| %zu | least to most %s? | | | |\n", i + 1, p.macro->name);
    } else {
      std::fprintf(scores.get(), "| %zu | which is more %s? | | | |\n", i + 1, p.macro->name);
    }
    std::fprintf(stderr, "\rblind %zu/%zu", i + 1, pairs.size());
  }
  std::fprintf(stderr, "\nblind: %zu pairs in %s\n", pairs.size(), outDir.c_str());
  return 0;
}
