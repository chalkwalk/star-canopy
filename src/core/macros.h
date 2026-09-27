#pragma once

#include "settings.h"

#include <map>
#include <string>

namespace starcanopy {

// The macros (PRINCIPLES §5, DESIGN.md §5): the controls a sky is steered by,
// each named for what it does to the sky and each made of raw dials.
//
// A macro runs from -1 to 1. At 0 it does nothing: every macro at 0 is the
// seed's own sky, exactly. At 1 the sky is what its name says (open, dense,
// luminous...), at -1 what its opposite says (enveloping, tenuous, brooding).
//
// Each binding moves one dial by `up` at 1 and by `down` at -1, and linearly
// between -- two slopes, since a dial seldom has as much room one way as the
// other. For a dial stepped by factors (Dial::geometric) the amounts are
// octaves, so a binding scales it; for any other, they are amounts of the
// dial's own units. A dial's value is its base -- its default, or the
// project's override -- plus every macro's amounts, times two to the power of
// every macro's octaves, clamped to its range and rounded if it is whole:
// additive, so one dial can serve several macros, as in Arps Euclidya.
//
// This table is the one definition of what a macro does (PRINCIPLES §13): the
// command line, the interface and the documentation all read it.

enum class Pull { Amount, Octaves };

struct Binding {
  const char* dial;
  Pull pull;
  float up;    // at +1
  float down;  // at -1: signed, usually the other way from up
};

struct Macro {
  const char* name;      // the sky at +1
  const char* opposite;  // the sky at -1
  const char* help;
  const Binding* bindings;
  int bindingCount;
};

// Every macro, in the order an interface shows them.
const Macro* macros(int& count);

// Macro values by name; a macro not named is at 0.
using MacroValues = std::map<std::string, float>;

// Sets the macro called `name` from its text. False, with the reason, for an
// unknown name, a malformed value or one outside -1..1.
bool setMacro(MacroValues& values, const std::string& name, const std::string& value,
              std::string& error);

// The dials, from their base values and the macros.
Settings resolveMacros(const Settings& base, const MacroValues& values);

}  // namespace starcanopy
