// The parameter study: how each raw dial moves the sky, for the triage and the
// macros made of the dials (ROADMAP.md, Macros). Results are CSV, one row per
// bake; docs/studies/parameters.md is what was made of them.
//
//   study oat    --style mass|shell [--size N] [--seeds 1,3,5,7] [--dials a,b] --out FILE
//     One at a time: every dial swept across its range from the defaults, on
//     each seed, each compared with that seed's default sky. What each dial
//     does alone: whether anything, how much, which way.
//
//   study morris --style mass|shell [--size N] [--trajectories R] [--dials a,b] --out FILE
//     Morris's elementary effects: trajectories from random points across a
//     usable space of the dials, near enough their defaults to be skies
//     someone might make (see interval()), stepping one dial at a time. A dial's mean
//     effect says how much it matters anywhere; the spread of its effects,
//     whether that depends on the other dials -- large spread and small effect
//     at the defaults is a dial that matters only together with others.
//
//   study macros --style mass|shell [--size N] [--seeds 1,3,5,7] [--dials a,b] --out FILE
//     Each macro (--dials names macros here) at -1, -1/2, 1/2 and 1 on each
//     seed, each compared with that seed's own sky: whether it moves the sky
//     the way its name says, on every seed, and how far (docs/studies/macros.md).
//
// Change is measured as the displayed sky's mean absolute difference, in 8-bit
// levels, solid-angle weighted (measure.h); a measure of how much, never of
// better (PRINCIPLES §2).

#include "bake.h"
#include "cubemap_target.h"
#include "gl_context.h"
#include "macros.h"
#include "measure.h"
#include "settings.h"
#include "sky.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace starcanopy;

namespace {

// Dials that trade time for fidelity rather than change what the sky is, and
// the seed. The oat sweep measures them too; morris leaves them at defaults.
const std::set<std::string> kQuality = {"step-frac", "max-steps", "light-res", "light-steps",
                                        "supersample", "denoise", "galaxy-res"};
// Views for judging the physics, not the look: the nebula off, the lines' own
// colours ungraded, and their astrophotographic mappings. The oat sweep
// measures them; morris leaves them at defaults, since one of them alone --
// the nebula off -- empties the sky.
const std::set<std::string> kDebug = {"nebula", "grade", "line-colors"};

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

std::string text(double v, bool integer) {
  char buf[32];
  if (integer) {
    std::snprintf(buf, sizeof(buf), "%ld", std::lround(v));
  } else {
    std::snprintf(buf, sizeof(buf), "%.6g", v);
  }
  return buf;
}

double current(const Settings& s, const Dial& d) {
  return d.real ? s.*d.real : s.*d.integer;
}

// The interval a dial is studied over. Wide, for the oat sweep, where the
// extremes are the point: a dial stepped by amounts over its whole range; one
// stepped by factors to four times either way of its default, its declared
// range running to extremes nobody would use; from its floor to sixty-four
// times it, if its default is off. Narrow, for morris, whose random points
// must be skies someone might make: a wide interval for every dial at once
// made nearly every point an empty sky (the median point was 100% clear in
// both styles). Narrow is a quarter of the range either side of the default,
// or a factor of two either way, or up to eight times the floor.
void interval(const Dial& d, double def, bool narrow, double& lo, double& hi) {
  if (!d.geometric) {
    double reach = narrow ? 0.25 * (d.hi - d.lo) : static_cast<double>(d.hi - d.lo);
    lo = narrow ? std::max(static_cast<double>(d.lo), def - reach) : d.lo;
    hi = narrow ? std::min(static_cast<double>(d.hi), def + reach) : d.hi;
    return;
  }
  double floor = std::max(static_cast<double>(d.floor), 1e-9), factor = narrow ? 2.0 : 4.0;
  if (def <= 0.0) {
    lo = floor;
    hi = std::min(static_cast<double>(d.hi), (narrow ? 8.0 : 64.0) * floor);
  } else {
    lo = std::max(std::max(static_cast<double>(d.lo), floor), def / factor);
    hi = std::min(static_cast<double>(d.hi), def * factor);
  }
}

// u in 0..1 across the dial's study interval, as the dial's value text.
std::string at(const Dial& d, double def, double u) {
  if (d.choices) {
    int i = std::min(d.choiceCount - 1, static_cast<int>(u * d.choiceCount));
    return d.choices[i];
  }
  double lo, hi;
  interval(d, def, true, lo, hi);
  double v = d.geometric ? lo * std::pow(hi / lo, u) : lo + u * (hi - lo);
  return text(v, d.integer != nullptr);
}

// The values the oat sweep bakes a dial at: every other choice; for a
// geometric dial its default over and times 2 and 4, and 0 if it may be off;
// otherwise five points across its range. Values equal to the default are
// skipped.
std::vector<std::string> sweep(const Dial& d, const Settings& defaults) {
  std::vector<std::string> out;
  if (d.choices) {
    for (int i = 0; i < d.choiceCount; i++) {
      if (i != defaults.*d.integer) {
        out.push_back(d.choices[i]);
      }
    }
    return out;
  }
  double def = current(defaults, d);
  std::vector<double> values;
  if (d.geometric) {
    double lo, hi;
    interval(d, def, false, lo, hi);
    if (def > 0.0) {
      for (double f : {0.25, 0.5, 2.0, 4.0}) {
        values.push_back(std::min(hi, std::max(lo, def * f)));
      }
      if (d.lo == 0.0f) {
        values.push_back(0.0);
      }
    } else {
      for (double f : {1.0, 4.0, 16.0, 64.0}) {
        values.push_back(std::min(hi, lo * f));
      }
    }
  } else {
    for (int k = 0; k < 5; k++) {
      values.push_back(d.lo + (d.hi - d.lo) * k / 4.0);
    }
  }
  std::set<std::string> seen{text(def, d.integer != nullptr)};
  for (double v : values) {
    std::string t = text(v, d.integer != nullptr);
    if (seen.insert(t).second) {
      out.push_back(t);
    }
  }
  return out;
}

struct Study {
  std::unique_ptr<GlContext> context;
  std::unique_ptr<Baker> baker;
  int size = 128;
  FILE* out = nullptr;

  Baked bake(const Settings& s, double& seconds) {
    auto start = std::chrono::steady_clock::now();
    CubemapTarget target(size);
    bakeSky(*baker, s, target);
    Baked b{target.read(), target.readTransmittance()};
    seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return b;
  }

  void header(const char* first) {
    std::fprintf(out,
                 "%s,dial,from,to,change,changed_share,brightness,contrast,clear,opaque,chroma,hue,"
                 "hue_spread,detail,bright_share,seconds\n",
                 first);
  }

  void row(const std::string& lead, const std::string& dial, const std::string& from,
           const std::string& to, double change, double share, const Descriptors& d,
           double seconds) {
    std::fprintf(out, "%s,%s,%s,%s,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.5g,%.3f\n",
                 lead.c_str(), dial.c_str(), from.c_str(), to.c_str(), change, share, d.brightness,
                 d.contrast, d.clear, d.opaque, d.chroma, d.hue, d.hueSpread, d.detail,
                 d.brightShare, seconds);
    std::fflush(out);
  }
};

std::vector<const Dial*> chosen(const std::vector<std::string>& names, bool withQuality) {
  int count = 0;
  const Dial* all = dials(count);
  std::vector<const Dial*> out;
  for (int i = 0; i < count; i++) {
    std::string name = all[i].name;
    // The seed, and the style, which is the study's own axis.
    if (all[i].seed || all[i].owner || name == "form" ||
        (!withQuality && (kQuality.count(name) || kDebug.count(name)))) {
      continue;
    }
    if (names.empty() || std::find(names.begin(), names.end(), name) != names.end()) {
      out.push_back(&all[i]);
    }
  }
  return out;
}

void oat(Study& st, const Settings& defaults, const std::vector<uint32_t>& seeds,
         const std::vector<const Dial*>& list) {
  st.header("seed");
  size_t total = 0, done = 0;
  for (const Dial* d : list) {
    total += sweep(*d, defaults).size() * seeds.size();
  }
  std::string error;
  for (uint32_t seed : seeds) {
    Settings base = defaults;
    base.seed = seed;
    double seconds;
    Baked ref = st.bake(base, seconds);
    st.row(std::to_string(seed), "(default)", "", "", 0.0, 0.0, describe(ref), seconds);
    for (const Dial* d : list) {
      std::string from = dialValue(base, *d);
      for (const std::string& value : sweep(*d, defaults)) {
        Settings s = base;
        if (!setDial(s, d->name, value, error)) {
          std::fprintf(stderr, "study: %s\n", error.c_str());
          continue;
        }
        Baked b = st.bake(s, seconds);
        st.row(std::to_string(seed), d->name, from, value, change(ref.radiance, b.radiance),
               changedShare(ref.radiance, b.radiance), describe(b), seconds);
        std::fprintf(stderr, "\roat %zu/%zu", ++done, total);
      }
    }
  }
  std::fprintf(stderr, "\n");
}

void morris(Study& st, const Settings& defaults, int trajectories,
            const std::vector<const Dial*>& list) {
  st.header("trajectory,step,seed");
  std::mt19937 rng(20260926u);
  const int levels = 4;
  const double delta = levels / (2.0 * (levels - 1));  // 2/3: steps of two levels
  std::string error;
  size_t total = static_cast<size_t>(trajectories) * (list.size() + 1), done = 0;
  for (int t = 0; t < trajectories; t++) {
    // A random start: each dial at a level from which +delta stays in range,
    // and a random seed, so the effects are of the sky in general.
    std::vector<double> u(list.size());
    for (double& x : u) {
      x = std::uniform_int_distribution<int>(0, levels - 1 - 2)(rng) / double(levels - 1);
    }
    std::vector<size_t> order(list.size());
    for (size_t i = 0; i < order.size(); i++) {
      order[i] = i;
    }
    std::shuffle(order.begin(), order.end(), rng);
    Settings s = defaults;
    s.seed = std::uniform_int_distribution<uint32_t>(1, 99999)(rng);
    auto apply = [&](size_t i) {
      if (!setDial(s, list[i]->name, at(*list[i], current(defaults, *list[i]), u[i]), error)) {
        std::fprintf(stderr, "study: %s\n", error.c_str());
      }
    };
    for (size_t i = 0; i < list.size(); i++) {
      apply(i);
    }
    double seconds;
    Baked prev = st.bake(s, seconds);
    std::string lead = std::to_string(t) + ",0," + std::to_string(s.seed);
    st.row(lead, "(start)", "", "", 0.0, 0.0, describe(prev), seconds);
    std::fprintf(stderr, "\rmorris %zu/%zu", ++done, total);
    int step = 1;
    for (size_t i : order) {
      std::string from = dialValue(s, *list[i]);
      u[i] += delta;
      apply(i);
      Baked next = st.bake(s, seconds);
      lead = std::to_string(t) + "," + std::to_string(step++) + "," + std::to_string(s.seed);
      st.row(lead, list[i]->name, from, dialValue(s, *list[i]), change(prev.radiance, next.radiance),
             changedShare(prev.radiance, next.radiance), describe(next), seconds);
      prev = std::move(next);
      std::fprintf(stderr, "\rmorris %zu/%zu", ++done, total);
    }
  }
  std::fprintf(stderr, "\n");
}

void macroSweep(Study& st, const Settings& defaults, const std::vector<uint32_t>& seeds,
                const std::vector<std::string>& names) {
  st.header("seed");
  int count = 0;
  const Macro* m = macros(count);
  std::vector<const Macro*> list;
  for (int i = 0; i < count; i++) {
    if (names.empty() || std::find(names.begin(), names.end(), m[i].name) != names.end()) {
      list.push_back(&m[i]);
    }
  }
  const float values[] = {-1.0f, -0.5f, 0.5f, 1.0f};
  size_t total = seeds.size() * list.size() * std::size(values), done = 0;
  for (uint32_t seed : seeds) {
    Settings base = defaults;
    base.seed = seed;
    double seconds;
    Baked ref = st.bake(base, seconds);
    st.row(std::to_string(seed), "(default)", "", "", 0.0, 0.0, describe(ref), seconds);
    for (const Macro* macro : list) {
      for (float v : values) {
        Baked b = st.bake(resolveMacros(base, {{macro->name, v}}), seconds);
        st.row(std::to_string(seed), macro->name, "0", text(v, false), change(ref.radiance, b.radiance),
               changedShare(ref.radiance, b.radiance), describe(b), seconds);
        std::fprintf(stderr, "\rmacros %zu/%zu", ++done, total);
      }
    }
  }
  std::fprintf(stderr, "\n");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: study oat|morris|macros --style mass|shell [options] --out FILE\n");
    return 2;
  }
  std::string mode = argv[1], style = "mass", outPath, error;
  std::vector<uint32_t> seeds = {1, 3, 5, 7};
  std::vector<std::string> names;
  int trajectories = 8;
  Study st;
  for (int i = 2; i + 1 < argc; i += 2) {
    std::string arg = argv[i], value = argv[i + 1];
    if (arg == "--style") {
      style = value;
    } else if (arg == "--size") {
      st.size = std::atoi(value.c_str());
    } else if (arg == "--seeds") {
      seeds.clear();
      for (const std::string& s : split(value)) {
        seeds.push_back(static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10)));
      }
    } else if (arg == "--dials") {
      names = split(value);
    } else if (arg == "--trajectories") {
      trajectories = std::atoi(value.c_str());
    } else if (arg == "--out") {
      outPath = value;
    } else {
      std::fprintf(stderr, "study: unknown option %s\n", arg.c_str());
      return 2;
    }
  }
  Settings defaults;
  if (!setDial(defaults, "form", style, error) || outPath.empty()) {
    std::fprintf(stderr, "study: %s\n", outPath.empty() ? "--out is required" : error.c_str());
    return 2;
  }
  st.context = GlContext::create(ContextKind::Auto, error);
  if (!st.context) {
    std::fprintf(stderr, "study: no context: %s\n", error.c_str());
    return 1;
  }
  st.baker = std::make_unique<Baker>();
  if (!st.baker->ok(error)) {
    std::fprintf(stderr, "study: %s\n", error.c_str());
    return 1;
  }
  st.out = std::fopen(outPath.c_str(), "w");
  if (!st.out) {
    std::fprintf(stderr, "study: cannot write %s\n", outPath.c_str());
    return 1;
  }
  std::fprintf(stderr, "study: %s, %s, %d a face, on %s\n", mode.c_str(), style.c_str(), st.size,
               st.context->description().c_str());
  if (mode == "oat") {
    oat(st, defaults, seeds, chosen(names, true));
  } else if (mode == "macros") {
    macroSweep(st, defaults, seeds, names);
  } else if (mode == "morris") {
    morris(st, defaults, trajectories, chosen(names, false));
  } else {
    std::fprintf(stderr, "study: unknown mode %s\n", mode.c_str());
    return 2;
  }
  std::fclose(st.out);
  return 0;
}
