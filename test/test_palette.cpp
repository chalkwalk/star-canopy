// The palette space keeps its promises: every ramp stop has luminance 1 (the
// grade changes hue and never brightness), every palette's dark end lies in its
// family's range, the families lead as often as their weights say, and the same
// seed is the same palette.

#include "check.h"
#include "palette.h"

#include <cmath>
#include <cstdio>

using namespace starcanopy;

int main() {
  const float range[kFamilyCount][2] = {{17, 63}, {112, 152}, {182, 198}, {228, 258}};
  int drawn[kFamilyCount] = {};
  const int seeds = 4000;
  for (uint32_t seed = 1; seed <= seeds; seed++) {
    Palette p;
    int f = drawPalette(seed, -1, p);
    CHECK(f >= 0 && f < kFamilyCount);
    drawn[f]++;
    CHECK(p.hue >= range[f][0] && p.hue <= range[f][1]);
    Palette again;
    drawPalette(seed, -1, again);
    CHECK(again.hue == p.hue && again.chroma == p.chroma);

    float ramp[kRampStops][3];
    paletteRamp(p, ramp);
    for (auto& stop : ramp) {
      float lum = 0.2126f * stop[0] + 0.7152f * stop[1] + 0.0722f * stop[2];
      CHECK(std::fabs(lum - 1.0f) < 1e-4f);
      CHECK(stop[0] >= 0.0f && stop[1] >= 0.0f && stop[2] >= 0.0f);
    }
  }
  const float weight[kFamilyCount] = {0.57f, 0.13f, 0.09f, 0.21f};
  for (int f = 0; f < kFamilyCount; f++) {
    float share = static_cast<float>(drawn[f]) / seeds;
    std::printf("family %d: %.3f of seeds (weight %.2f)\n", f, share, weight[f]);
    CHECK(std::fabs(share - weight[f]) < 0.03f);
  }

  // A family asked for is the family given.
  for (int f = 0; f < kFamilyCount; f++) {
    Palette p;
    CHECK(drawPalette(99, f, p) == f);
    CHECK(p.hue >= range[f][0] && p.hue <= range[f][1]);
  }
  return test::finish();
}
