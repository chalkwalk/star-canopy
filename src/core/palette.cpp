#include "palette.h"

#include <algorithm>
#include <cmath>

namespace starcanopy {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// The hue dust turns toward, the short way round.
constexpr float kBrown = 55.0f;

// The families a palette's dark end is drawn from, fitted to the paths of hue
// and chroma against lightness in 23 reference images, with the gaps between
// them that no reference uses: nothing starts yellow, nothing lies between teal
// and blue, nothing is pink or purple. Chroma peaks in the middle tones and is
// warm's to spend; the cool families are muted. The weights are how often each
// family leads in the references; dust, how far each family's dust moves
// toward brown.
const struct {
  float hue[2], chroma[2], dust[2], weight;
} kFamily[kFamilyCount] = {
  {{17.0f, 63.0f}, {0.08f, 0.19f}, {0.3f, 0.6f}, 0.57f},    // warm
  {{112.0f, 152.0f}, {0.03f, 0.10f}, {0.6f, 1.0f}, 0.13f},  // green
  {{182.0f, 198.0f}, {0.03f, 0.08f}, {0.5f, 0.9f}, 0.09f},  // teal
  {{228.0f, 258.0f}, {0.03f, 0.10f}, {0.0f, 0.2f}, 0.21f},  // blue
};

// A stream of uniform numbers 0..1 from a hash state.
float seedUniform(uint32_t& h) {
  h ^= h >> 15;
  h *= 0x2c1b3c6du;
  h ^= h >> 12;
  h *= 0x297a2d39u;
  h ^= h >> 15;
  return static_cast<float>(h >> 8) / 16777216.0f;
}

float span(const float r[2], float u) {
  return r[0] + (r[1] - r[0]) * u;
}

// A palette of family f, its values drawn from the family's ranges. Warm,
// green and teal turn with lightness, from somewhere between 0.25 and 0.45, to
// meet at a yellow-white of 92-110 degrees; blue stays blue, easing 8-32
// degrees toward cyan.
void drawFrom(int f, uint32_t& h, Palette& g) {
  const auto& p = kFamily[f];
  g.hue = span(p.hue, seedUniform(h));
  g.top = f == kBlue ? g.hue - 8.0f - 24.0f * seedUniform(h) : 92.0f + 18.0f * seedUniform(h);
  g.drift = 1.0f;
  g.start = 0.25f + 0.2f * seedUniform(h);
  g.chroma = p.chroma[0] * powf(p.chroma[1] / p.chroma[0], seedUniform(h));
  g.peak = 0.5f + 0.2f * seedUniform(h);
  g.dust = span(p.dust, seedUniform(h));
}

// OKLab to linear sRGB, Björn Ottosson's matrices.
void oklabToLinear(float l, float a, float b, float rgb[3]) {
  float l_ = l + 0.3963377774f * a + 0.2158037573f * b;
  float m_ = l - 0.1055613458f * a - 0.0638541728f * b;
  float s_ = l - 0.0894841775f * a - 1.2914855480f * b;
  l_ = l_ * l_ * l_;
  m_ = m_ * m_ * m_;
  s_ = s_ * s_ * s_;
  rgb[0] = 4.0767416621f * l_ - 3.3077115913f * m_ + 0.2309699292f * s_;
  rgb[1] = -1.2684380046f * l_ + 2.6097574011f * m_ - 0.3413193965f * s_;
  rgb[2] = -0.0041960863f * l_ - 0.7034186147f * m_ + 1.7076147010f * s_;
}

// The dust's palette from the gas's: its hue moved toward brown by the
// palette's dust, the short way round; its chroma kept up at the dark end,
// where the gas's fades to grey, since dark brown is still brown -- but where
// the dust keeps the gas's hue, as in a blue sky, less colourful than the gas,
// dark slate rather than navy; its peak darker, the dust being the dark part of
// the sky; and its lit edges drifting to cream like the gas's highlights.
Palette dustPalette(const Palette& g) {
  float turn = fmodf(kBrown - g.hue + 540.0f, 360.0f) - 180.0f;
  Palette d = g;
  d.hue = g.hue + g.dust * turn;
  d.chroma = 0.6f * g.chroma + g.dust * (fmaxf(g.chroma, 0.05f) - 0.6f * g.chroma);
  d.peak = 0.42f;
  d.drift = fmaxf(g.drift, 0.8f * g.dust);
  return d;
}

}  // namespace

int drawPalette(uint32_t seed, int family, Palette& p) {
  uint32_t h = seed * 2654435761u + 0x9e3779b9u;
  float u = seedUniform(h), acc = 0.0f;
  int f = family;
  if (f < 0) {
    for (f = 0; f < kFamilyCount - 1; f++) {
      acc += kFamily[f].weight;
      if (u < acc) {
        break;
      }
    }
  }
  drawFrom(f, h, p);
  return f;
}

void paletteRamp(const Palette& g, float ramp[kRampStops][3]) {
  for (int i = 0; i < kRampStops; i++) {
    float t = (i + 0.5f) / kRampStops;
    // Displayed lightness to OKLab's: sRGB decoded, then its cube root, which is
    // OKLab's L for a grey.
    float y = t <= 0.04045f ? t / 12.92f : powf((t + 0.055f) / 1.055f, 2.4f);
    float l = cbrtf(y), d = l - g.peak, w = d < 0.0f ? 0.32f : 0.25f;
    float c = g.chroma * expf(-(d * d) / (w * w));
    float s = (l - g.start) / (0.95f - g.start);
    float along = g.drift * (s <= 0.0f ? 0.0f : s >= 1.0f ? 1.0f : s * s * (3.0f - 2.0f * s));
    float hue = g.hue + along * (g.top - g.hue);
    oklabToLinear(l, c * cosf(hue * kPi / 180.0f), c * sinf(hue * kPi / 180.0f), ramp[i]);
    for (int j = 0; j < 3; j++) {
      ramp[i][j] = ramp[i][j] > 0.0f ? ramp[i][j] : 0.0f;
    }
    float lum = 0.2126f * ramp[i][0] + 0.7152f * ramp[i][1] + 0.0722f * ramp[i][2];
    for (int j = 0; j < 3; j++) {
      ramp[i][j] /= lum > 1e-6f ? lum : 1e-6f;
    }
  }
}

void buildGrade(uint32_t seed, const PaletteChoice& choice, Look& l) {
  Palette g{};
  int family = 0;
  if (!choice.grade) {
    for (int i = 0; i < kRampStops; i++) {
      for (int j = 0; j < 3; j++) {
        l.ramp[i][j] = l.dustRamp[i][j] = 1.0f;
      }
    }
    l.grade = 0.0f;
  } else {
    family = drawPalette(seed, choice.family, g);
    paletteRamp(g, l.ramp);
    paletteRamp(dustPalette(g), l.dustRamp);
    // A dust setting of 0 is the gas's ramp for the dust too.
    for (int i = 0; i < kRampStops; i++) {
      for (int j = 0; j < 3; j++) {
        l.dustRamp[i][j] = l.ramp[i][j] + choice.dust * (l.dustRamp[i][j] - l.ramp[i][j]);
      }
    }
    l.grade = choice.strength;
  }
  (void)family;
}

}  // namespace starcanopy
