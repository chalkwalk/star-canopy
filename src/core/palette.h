#pragma once

#include "look.h"

#include <cstdint>

namespace starcanopy {

// The palette space the sky's colour is drawn from (DESIGN.md §4.1), fitted to
// measurements of the reference skies; provenance in docs/references/SOURCES.md.
//
// Every reference is a single curve through colour: within a band of lightness
// its hue hardly varies, and every curve has the same shape --
//
//   chroma  an arch, low in the shadows, peaking in the midtones, low again in
//           the highlights
//   hue     drifting toward a yellow-white as the gas brightens, as a lit
//           cloud's highlights go cream; from the orange side and the green
//           side alike. Except the blues, which keep their hue.
//
// So a palette is a few numbers: the hue in the shadows, the peak chroma, the
// lightness it peaks at, and how far toward the highlight hue it has turned by
// the highlights. Drawing those numbers from the spread the references show,
// rather than copying any one reference's curve, is what keeps a generated sky
// of their kind without being one of them (fence #4).
struct Palette {
  float hue;     // in the shadows, OKLCh degrees
  float chroma;  // at its peak
  float peak;    // the OKLab lightness it peaks at
  float drift;   // how far toward `top` by the highlights, 0..1
  float dust;    // how far the dust's hue is from the gas's toward brown, 0..1
  float top;     // the hue the highlights turn toward, OKLCh degrees
  float start;   // the OKLab lightness the turn begins at
};

enum PaletteFamily { kWarm, kGreen, kTeal, kBlue, kFamilyCount };

struct PaletteChoice {
  bool grade = true;      // false: the lines' own physical colours
  int family = -1;        // a PaletteFamily, or -1 to draw one by seed
  float strength = 1.0f;  // how far toward the grade, 0..1
  float dust = 1.0f;      // how far the dust takes its own colour, 0..1
};

// A palette of the family for this seed, or of a family drawn by the weights
// the references lead with. Returns the family.
int drawPalette(uint32_t seed, int family, Palette& p);

// The ramp for a palette: at each stop's displayed lightness, the colour of
// that lightness, made linear and scaled to luminance 1, since the grade takes
// only its colour.
void paletteRamp(const Palette& p, float ramp[kRampStops][3]);

// The grade's ramps and strength, and its second palette, into the look.
void buildGrade(uint32_t seed, const PaletteChoice& choice, Look& look);

}  // namespace starcanopy
