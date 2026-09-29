// The dials: one table, every name unique, every default in its own range, and
// every value set by name read back the same.

#include "check.h"
#include "settings.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>

using namespace starcanopy;

int main() {
  int count = 0;
  const Dial* d = dials(count);
  Settings defaults;
  std::set<std::string> names;
  for (int i = 0; i < count; i++) {
    CHECK(names.insert(d[i].name).second);
    CHECK((d[i].real != nullptr) + (d[i].integer != nullptr) + (d[i].seed != nullptr) == 1);
    // Its default in range -- or auto, the seed's, for a dial that may be.
    if (d[i].real && !(d[i].automatic && std::isnan(defaults.*d[i].real))) {
      float v = defaults.*d[i].real;
      if (v < d[i].lo || v > d[i].hi) {
        std::printf("%s: default %g outside %g..%g\n", d[i].name, v, d[i].lo, d[i].hi);
      }
      CHECK(v >= d[i].lo && v <= d[i].hi);
    }
    // A dial stepped by factors has somewhere above zero to step from, and its
    // default is off or on that scale.
    if (d[i].geometric) {
      double v = d[i].real ? defaults.*d[i].real : defaults.*d[i].integer;
      CHECK(d[i].floor > 0.0f && d[i].floor <= d[i].hi);
      CHECK(v == 0.0 || v >= d[i].floor);
      CHECK(d[i].lo == 0.0f || d[i].lo == d[i].floor);
    }
    // A dial a macro owns is not set on its own; the rest of these checks are
    // for the dials that are.
    if (d[i].owner) {
      Settings e;
      std::string why;
      CHECK(!setDial(e, d[i].name, dialValue(defaults, d[i]), why) &&
            why.find(d[i].owner) != std::string::npos);
      continue;
    }
    // Its own range's ends, written as a person writes them, are in range.
    if (!d[i].choices && !d[i].seed) {
      char lo[32], hi[32];
      std::snprintf(lo, sizeof(lo), "%g", static_cast<double>(d[i].lo));
      std::snprintf(hi, sizeof(hi), "%g", static_cast<double>(d[i].hi));
      Settings e;
      std::string why;
      bool ok = setDial(e, d[i].name, lo, why) && setDial(e, d[i].name, hi, why);
      if (!ok) {
        std::printf("%s\n", why.c_str());
      }
      CHECK(ok);
    }
    // Round trip: its own text sets it to the same value.
    Settings s;
    std::string error, text = dialValue(defaults, d[i]);
    CHECK(setDial(s, d[i].name, text, error));
    CHECK(dialValue(s, d[i]) == text);
  }

  Settings s;
  std::string error;
  CHECK(setDial(s, "seed", "4294967295", error) && s.seed == 4294967295u);
  CHECK(!setDial(s, "seed", "4294967296", error));
  CHECK(!setDial(s, "seed", "-1", error));
  CHECK(setDial(s, "density", "2.5", error) && s.density == 2.5f);
  CHECK(!setDial(s, "density", "1000", error));  // out of range
  CHECK(!setDial(s, "density", "fast", error));
  CHECK(!setDial(s, "density", "", error));
  CHECK(!setDial(s, "no-such-dial", "1", error));
  CHECK(setDial(s, "line-colors", "hoo", error) && s.lineColors == 2);
  CHECK(!setDial(s, "line-colors", "purple", error));
  CHECK(!setDial(s, "max-steps", "1.5", error));
  // A dial that may be auto takes a number or auto; one that may not, not auto.
  CHECK(setDial(s, "galaxy-radius", "2", error) && s.galaxyRadius == 2.0f);
  CHECK(setDial(s, "galaxy-radius", "auto", error) && std::isnan(s.galaxyRadius));
  CHECK(!setDial(s, "density", "auto", error));
  // The place is the galactic macro's.
  CHECK(!setDial(s, "galaxy-place", "0.5", error) && error.find("galactic") != std::string::npos);

  // The same dials make the same sky description.
  Sky a = buildSky(defaults), b = buildSky(defaults);
  CHECK(a.look.exposure == b.look.exposure && a.scene.seed == b.scene.seed);
  return test::finish();
}
