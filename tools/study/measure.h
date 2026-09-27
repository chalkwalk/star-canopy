#pragma once

#include "cubemap.h"

#include <array>
#include <vector>

namespace starcanopy {

// What a baked sky is like, in numbers, for the parameter study. These are
// instruments for EXPLAINING how a dial moves the sky, not judgements of
// whether it is better (PRINCIPLES §2): none of them has been checked against
// blind scores. Every one is over the whole sphere, each texel weighted by the
// solid angle it covers, and on the sky as displayed -- through the display
// curve the look was judged through -- except where it says otherwise.
struct Descriptors {
  double brightness;   // mean displayed lightness, 0..1 (OKLab L)
  double contrast;     // its standard deviation
  double clear;        // share of the sky the gas hides almost nothing of (transmittance > 0.9)
  double opaque;       // share it hides almost everything of (transmittance < 0.1)
  double chroma;       // mean OKLab chroma: how colourful
  double hue;          // chroma-weighted mean hue, degrees
  double hueSpread;    // 1 - the resultant length of those hues: 0 one hue, 1 all of them
  double detail;       // mean |lightness - mean of its four neighbours|: fine structure
  double brightShare;  // share of the linear light in the brightest tenth of the sky
};

struct Baked {
  Cubemap radiance;
  std::array<std::vector<float>, 6> transmittance;
};

Descriptors describe(const Baked& sky);

// How much two skies differ as displayed: the solid-angle-weighted mean of
// |a - b| over texels and channels, in 8-bit display levels.
double change(const Cubemap& a, const Cubemap& b);

// The share of the sky, by solid angle, where the two differ visibly: by more
// than two 8-bit levels in some channel. change() averages over the whole
// sphere and so hardly sees a change to points -- the stars -- or to fine
// structure; this sees how much of the sky changed at all.
double changedShare(const Cubemap& a, const Cubemap& b);

}  // namespace starcanopy
