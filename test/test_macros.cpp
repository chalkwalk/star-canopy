// The macro table and its resolution (DESIGN.md §5): every macro at 0 is the
// seed's own sky, exactly; contributions add, amounts before octaves, and are
// clamped to the dial's range; and the table binds only real look dials.

#include "check.h"
#include "macros.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>

using namespace starcanopy;

namespace {

const Dial* find(const char* name) {
  int count = 0;
  const Dial* d = dials(count);
  for (int i = 0; i < count; i++) {
    if (std::string(d[i].name) == name) {
      return &d[i];
    }
  }
  return nullptr;
}

bool same(const Settings& a, const Settings& b) {
  int count = 0;
  const Dial* d = dials(count);
  bool ok = true;
  for (int i = 0; i < count; i++) {
    if (dialValue(a, d[i]) != dialValue(b, d[i])) {
      std::printf("  %s: %s, not %s\n", d[i].name, dialValue(b, d[i]).c_str(),
                  dialValue(a, d[i]).c_str());
      ok = false;
    }
  }
  return ok;
}

bool near(double a, double b) {
  return std::fabs(a - b) <= 1e-5 * std::fmax(1.0, std::fabs(b));
}

}  // namespace

int main() {
  int count = 0;
  const Macro* m = macros(count);
  CHECK(count >= 10 && count <= 20);

  // The table: names unique and not a dial's, so a macro and a raw dial are
  // never confused; each binding a numeric dial the macros may move, at most
  // once per macro, octaves only on dials stepped by factors; and each end of
  // each macro moving something.
  const std::set<std::string> notForMacros = {
    // quality: time for fidelity, not the look
    "step-frac", "max-steps", "light-res", "light-steps", "supersample", "denoise", "galaxy-res",
    // debug views
    "nebula", "grade", "line-colors",
    // off by a decision
    "spike", "spike-flux", "mass-edge", "mass-edge-patch",
    // no visible effect (docs/studies/parameters.md, Points)
    "external-galaxies",
    // the composition's own
    "seed"};
  std::set<std::string> names;
  for (int i = 0; i < count; i++) {
    CHECK(names.insert(m[i].name).second);
    CHECK(names.insert(m[i].opposite).second);
    CHECK(!find(m[i].name) && !find(m[i].opposite));
    CHECK(m[i].bindingCount > 0);
    std::set<std::string> bound;
    bool up = false, down = false;
    for (int k = 0; k < m[i].bindingCount; k++) {
      const Binding& b = m[i].bindings[k];
      const Dial* d = find(b.dial);
      if (!d) {
        std::printf("%s binds no dial called %s\n", m[i].name, b.dial);
        test::failures++;
        continue;
      }
      if (notForMacros.count(b.dial)) {
        std::printf("%s binds %s, which is not for macros\n", m[i].name, b.dial);
        test::failures++;
      }
      CHECK(!d->choices && !d->seed);
      CHECK(bound.insert(b.dial).second);
      CHECK(b.pull == Pull::Amount || d->geometric);
      up = up || b.up != 0.0f;
      down = down || b.down != 0.0f;
    }
    CHECK(up && down);
  }

  // Every macro at 0, named or not, is the sky as it was, bit for bit.
  Settings base;
  base.seed = 7;
  CHECK(same(base, resolveMacros(base, {})));
  MacroValues zero;
  for (int i = 0; i < count; i++) {
    zero[m[i].name] = 0.0f;
  }
  CHECK(same(base, resolveMacros(base, zero)));

  // Each end of each macro moves the dials it binds, and nothing else.
  for (int i = 0; i < count; i++) {
    for (float v : {1.0f, -1.0f}) {
      Settings s = resolveMacros(base, {{m[i].name, v}});
      int dialCount = 0;
      const Dial* d = dials(dialCount);
      for (int j = 0; j < dialCount; j++) {
        bool isBound = false;
        float amount = 0.0f;
        for (int k = 0; k < m[i].bindingCount; k++) {
          if (std::string(m[i].bindings[k].dial) == d[j].name) {
            isBound = true;
            amount = v > 0.0f ? m[i].bindings[k].up : m[i].bindings[k].down;
          }
        }
        bool moved = dialValue(s, d[j]) != dialValue(base, d[j]);
        if (moved != (isBound && amount != 0.0f)) {
          std::printf("%s at %g: %s %s\n", m[i].name, static_cast<double>(v), d[j].name,
                      moved ? "moved, unbound" : "bound, did not move");
          test::failures++;
        }
      }
    }
  }

  // Amounts and octaves, along two slopes: bright moves exposure by an octave
  // at 1 and at -1, half as far at a half.
  CHECK(near(resolveMacros(base, {{"bright", 1.0f}}).exposure, 0.36));
  CHECK(near(resolveMacros(base, {{"bright", -1.0f}}).exposure, 0.09));
  CHECK(near(resolveMacros(base, {{"bright", 0.5f}}).exposure, 0.18 * std::sqrt(2.0)));
  // open moves mass-inner +0.12 at 1 but -0.2 at -1.
  CHECK(near(resolveMacros(base, {{"open", 1.0f}}).massInner, 0.87));
  CHECK(near(resolveMacros(base, {{"open", -0.5f}}).massInner, 0.65));
  // From the base, not the default: an override is what the macros move.
  Settings over = base;
  over.exposure = 1.0f;
  CHECK(near(resolveMacros(over, {{"bright", 1.0f}}).exposure, 2.0));

  // Two macros on one dial add: open and fragmented both take from keep.
  float keep = resolveMacros(base, {{"open", 1.0f}, {"fragmented", 1.0f}}).keep;
  CHECK(near(keep, 0.7 - 0.15 - 0.1));
  // Octaves add too: bright raises the stars' brightness, starry lowers it.
  CHECK(near(resolveMacros(base, {{"bright", 1.0f}, {"starry", 0.5f}}).starBrightness,
             0.55 * std::exp2(0.25)));
  // A dial off by default is moved by amounts: luminous lights the cavity.
  CHECK(base.massCavity == 0.0f);
  CHECK(near(resolveMacros(base, {{"luminous", 1.0f}}).massCavity, 0.003));
  CHECK(resolveMacros(base, {{"luminous", -1.0f}}).massCavity == 0.0f);

  // A dial a macro owns: vivid runs the palettes' chroma from grey to twice.
  // No other macro binds it.
  CHECK(resolveMacros(base, {{"vivid", -1.0f}}).gradeChroma == 0.0f);
  CHECK(resolveMacros(base, {{"vivid", 1.0f}}).gradeChroma == 2.0f);
  {
    int dialCount = 0;
    const Dial* d = dials(dialCount);
    for (int j = 0; j < dialCount; j++) {
      int boundBy = 0;
      bool byOwner = false;
      for (int i = 0; i < count; i++) {
        for (int k = 0; k < m[i].bindingCount; k++) {
          if (std::string(m[i].bindings[k].dial) == d[j].name) {
            boundBy++;
            byOwner = byOwner || (d[j].owner && std::string(d[j].owner) == m[i].name);
          }
        }
      }
      if (d[j].owner && !(boundBy == 1 && byOwner)) {
        std::printf("%s belongs to %s, and should be bound by it alone\n", d[j].name, d[j].owner);
        test::failures++;
      }
    }
  }

  // Clamped to the dial's range, and whole dials rounded.
  over = base;
  over.viewerOffset = 3.9f;
  CHECK(resolveMacros(over, {{"open", 1.0f}}).viewerOffset == 4.0f);
  over.massInner = 0.2f;
  CHECK(resolveMacros(over, {{"open", -1.0f}}).massInner == 0.1f);
  CHECK(resolveMacros(base, {{"starry", 0.3f}}).starCount == static_cast<int>(std::lround(30000 * std::exp2(0.6))));

  // Values: -1..1, by name, with reasons.
  MacroValues v;
  std::string error;
  CHECK(setMacro(v, "open", "-0.25", error) && v["open"] == -0.25f);
  CHECK(setMacro(v, "open", "1", error) && setMacro(v, "open", "-1", error));
  CHECK(!setMacro(v, "open", "1.01", error) && error.find("enveloping") != std::string::npos);
  CHECK(!setMacro(v, "open", "wide", error));
  CHECK(!setMacro(v, "open", "", error));
  CHECK(!setMacro(v, "opne", "0.5", error) && error.find("opne") != std::string::npos);
  CHECK(!setMacro(v, "density", "0.5", error));
  return test::finish();
}
