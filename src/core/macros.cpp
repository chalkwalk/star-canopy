#include "macros.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace starcanopy {

namespace {

// The first macro set, made from the parameter study (docs/studies/parameters.md)
// and measured in docs/studies/macros.md. Only look dials are bound: never the
// quality dials, which are not the look, the debug views, the dials off by a
// decision, or those that retire with the shell. The amounts are where each
// macro, at its ends, still makes skies someone might want; which way is
// better is not theirs to say, and is scored blind (PRINCIPLES §2).
constexpr Pull A = Pull::Amount;
constexpr Pull O = Pull::Octaves;

// How much of the sky the gas covers: the viewer toward the mass's edge, its
// inner surface further out and its lobes drawn back, and fewer holes kept.
const Binding kOpen[] = {
  {"viewer-offset", A, 0.25f, -0.25f},
  {"mass-inner", A, 0.12f, -0.2f},
  {"mass-lobes", A, -0.2f, 0.2f},
  {"keep", A, -0.15f, 0.15f},
};

// How much the gas hides where it is: all of it, and the mass, denser.
const Binding kDense[] = {
  {"density", O, 1.0f, -1.0f},
  {"mass-density", O, 0.5f, -0.5f},
};

// Broken into pieces: column density spread to clear gaps and solid clouds,
// the detail eating further in, more and smaller holes.
const Binding kFragmented[] = {
  {"contrast", A, 0.8f, -0.8f},
  {"erosion", A, 0.25f, -0.3f},
  {"hole-scale", O, 1.0f, -1.0f},
  {"keep", A, -0.1f, 0.1f},
};

// Fine structure: rougher octaves starting finer, the mass's lumps and their
// shadows toward the light.
const Binding kDetailed[] = {
  {"detail-gain", A, 0.15f, -0.2f},
  {"detail-scale", O, 1.0f, -1.0f},
  {"mass-fine", A, 1.0f, -0.7f},
  {"graze", A, 1.0f, -1.5f},
};

// Deep rounded lumps with firm edges, like cumulus; wispy is the
// lumps flattened and their edges eaten into ridged, swirled strands. The
// lumps' depth is the macro's own dial (mass-billow): without it, billowing
// could only trade billowed detail for ridged, which the scorer could barely
// see in two rounds. Not more fine lumps, which crumple the billows rather
// than round them; and only a little larger, since large deep lumps pull the
// mass apart into clumps in clear sky, a second open.
const Binding kBillowing[] = {
  {"mass-billow", A, 0.8f, -0.6f},
  {"filament", A, -0.45f, 0.45f},
  {"mass-scale", O, -0.2f, 0.3f},
  {"erosion", A, -0.3f, 0.25f},
  {"hardness", A, 0.2f, 0.0f},
  {"mass-warp", A, 0.0f, 0.3f},
};

// Hard edges and hard shadows: crisp detail, small clusters, a sharp fill,
// and ionisation fronts sharpened by opaque gas.
const Binding kCrisp[] = {
  {"hardness", A, 0.35f, -0.35f},
  {"mass-cluster-size", O, -1.5f, 1.5f},
  {"fill-shadow", O, 1.0f, -1.0f},
  {"ion-opacity", O, 1.0f, -1.0f},
};

// Light from within: brighter clusters, a glowing cavity in front, more fill
// on the faces turned away.
const Binding kLuminous[] = {
  {"luminosity", O, 0.6f, -1.0f},
  {"mass-cavity", A, 0.003f, 0.0f},
  {"fill", O, 0.7f, -1.0f},
};

// The whole sky brighter, as a longer exposure would make it.
const Binding kBright[] = {
  {"exposure", O, 1.0f, -1.0f},
  {"star-brightness", O, 0.5f, -0.5f},
  {"galaxy-glow", O, 0.5f, -0.5f},
};

// A glow over everything, and dust lit by the stars; clear is black between.
const Binding kHazy[] = {
  {"haze", O, 2.5f, -2.0f},
  {"reflection", O, 1.0f, -1.0f},
};

// Colour: the palettes' chroma, from grey at -1 to twice their own at 1, and
// the stars and the galaxy drawn further into the grade either way, so a muted
// sky is muted all through. Not by grade-strength, which gives the colour back
// to the lines' own, more colourful than most palettes, not less.
const Binding kVivid[] = {
  {"grade-chroma", A, 1.0f, -1.0f},
  {"grade-stars", A, 0.2f, 0.3f},
  {"grade-galaxy", A, 0.2f, 0.3f},
};

// More stars, and further ones still points; each dimmer, so that many are a
// field and not a glare (blind, round 1: "too many stars, at the brightness").
const Binding kStarry[] = {
  {"star-count", O, 2.0f, -2.0f},
  {"star-brightness", O, -0.5f, 0.0f},
  {"star-reach", O, 0.5f, -0.5f},
};


// Buckled, swirled and blown out; calm is smooth and whole. Named violent in
// its first two rounds, and picked by name every time, but the scorer could
// not say what violent meant; turbulent says it.
const Binding kTurbulent[] = {
  {"fold", A, 0.15f, -0.15f},
  {"mass-warp", A, 0.25f, -0.2f},
  {"mass-blister", A, 0.25f, -0.25f},
  {"fold-scale", O, 0.7f, -0.7f},
};


#define MACRO(name, opposite, help, b) Macro{name, opposite, help, b, static_cast<int>(std::size(b))}

const Macro kMacros[] = {
  MACRO("open", "enveloping", "how much of the sky the gas covers", kOpen),
  MACRO("dense", "tenuous", "how much the gas hides where it is", kDense),
  MACRO("fragmented", "whole", "gas broken into pieces with clear sky between", kFragmented),
  MACRO("detailed", "smooth", "fine structure in the gas", kDetailed),
  MACRO("billowing", "wispy", "rounded billows, or ridged filaments", kBillowing),
  MACRO("crisp", "soft", "hard edges and hard shadows", kCrisp),
  MACRO("turbulent", "calm", "gas buckled, swirled and blown out", kTurbulent),
  MACRO("luminous", "brooding", "light from within the nebula", kLuminous),
  MACRO("bright", "dim", "the whole sky, as an exposure", kBright),
  MACRO("hazy", "clear", "a glow over the sky, the dark between not black", kHazy),
  MACRO("vivid", "muted", "how colourful", kVivid),
  MACRO("starry", "sparse", "how many stars, and how bright", kStarry),
};

#undef MACRO

const Dial* findDial(const char* name) {
  int count = 0;
  const Dial* d = dials(count);
  for (int i = 0; i < count; i++) {
    if (std::string(d[i].name) == name) {
      return &d[i];
    }
  }
  return nullptr;
}

}  // namespace

const Macro* macros(int& count) {
  count = static_cast<int>(std::size(kMacros));
  return kMacros;
}

bool setMacro(MacroValues& values, const std::string& name, const std::string& value,
              std::string& error) {
  for (const Macro& m : kMacros) {
    if (name != m.name) {
      continue;
    }
    char* end = nullptr;
    errno = 0;
    double v = std::strtod(value.c_str(), &end);
    if (value.empty() || *end || errno || !std::isfinite(v)) {
      error = name + ": '" + value + "' is not a number";
      return false;
    }
    if (v < -1.0 || v > 1.0) {
      error = name + ": " + value + " is outside -1 (" + m.opposite + ") .. 1 (" + m.name + ")";
      return false;
    }
    values[name] = static_cast<float>(v);
    return true;
  }
  error = "no macro called '" + name + "'";
  return false;
}

Settings resolveMacros(const Settings& base, const MacroValues& values) {
  struct Sum {
    const Dial* dial;
    double amount = 0.0, octaves = 0.0;
  };
  std::vector<Sum> sums;
  for (const Macro& m : kMacros) {
    auto it = values.find(m.name);
    float v = it == values.end() ? 0.0f : it->second;
    if (v == 0.0f) {
      continue;
    }
    for (int i = 0; i < m.bindingCount; i++) {
      const Binding& b = m.bindings[i];
      const Dial* d = findDial(b.dial);
      if (!d || !(d->real || d->integer) || d->choices) {
        continue;  // test_macros holds the table to real dials
      }
      Sum* s = nullptr;
      for (Sum& e : sums) {
        s = e.dial == d ? &e : s;
      }
      if (!s) {
        sums.push_back(Sum{d});
        s = &sums.back();
      }
      double by = v > 0.0f ? v * b.up : -v * b.down;
      (b.pull == Pull::Octaves ? s->octaves : s->amount) += by;
    }
  }
  Settings out = base;
  for (const Sum& s : sums) {
    const Dial& d = *s.dial;
    double x = d.real ? out.*d.real : out.*d.integer;
    x = (x + s.amount) * std::exp2(s.octaves);
    x = std::fmin(std::fmax(x, static_cast<double>(d.lo)), static_cast<double>(d.hi));
    if (d.integer) {
      out.*d.integer = static_cast<int>(std::lround(x));
    } else {
      out.*d.real = static_cast<float>(x);
    }
  }
  return out;
}

}  // namespace starcanopy
