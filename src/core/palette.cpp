#include "palette.h"

#include <algorithm>
#include <cmath>

namespace starcanopy {

namespace {

constexpr float kPi = 3.14159265358979323846f;


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

// A second palette, for a fan of colour across the sky or two regions of their
// own: 6 and 2 of the 23 references. Blended in over part of the sky by a
// smooth field of three seeded waves: softly and over less of it for a fan,
// whose second hue lives in the darks and middle tones, more sharply and over
// more for two regions, each a whole palette to near its top. The share is
// found by sampling the field, so it is the share of the sky.
//
// A fan's second palette is its first with the dark end's hue moved 35-75
// degrees along the strip the references use: in every one of their fans the
// second hue is the first's neighbour, and a fan of two families far apart puts
// hard lines of colour across the sky. Two regions pair by colour, warm against
// cool; but a split wants a break in the gas to change colour at, and the field
// knows nothing of the gas, so the seed never picks one -- only hue-type does.
void buildSecond(uint32_t seed, const PaletteChoice& choice, const Palette& g, int family,
                 Look& l) {
  static const int pair[kFamilyCount][2] = {
    {kBlue, kTeal},  // warm
    {kWarm, kTeal},  // green
    {kWarm, kWarm},  // teal
    {kWarm, kWarm},  // blue
  };
  uint32_t h = seed * 2654435761u + 0x1b873593u;
  float u = seedUniform(h);
  for (int k = 0; k < 3; k++) {
    float z = 2.0f * seedUniform(h) - 1.0f, a = 2.0f * kPi * seedUniform(h);
    float r = sqrtf(fmaxf(1.0f - z * z, 0.0f));
    l.hueWave[k][0] = r * cosf(a);
    l.hueWave[k][1] = z;
    l.hueWave[k][2] = r * sinf(a);
    l.hueWave[k][3] = 1.2f + 1.8f * seedUniform(h);
    l.huePhase[k] = 2.0f * kPi * seedUniform(h);
  }
  int type = choice.hueType;
  if (type == 0) {
    type = u < 0.65f ? 1 : 2;
  }
  l.ramp2Share = 0.0f;
  if (!choice.grade || type == 1) {
    return;
  }
  Palette g2;
  if (type == 2) {
    float turn = 35.0f + 40.0f * seedUniform(h);
    // Toward whichever end of the strip has room for it, 17 to 258 degrees.
    if (g.hue + turn > 258.0f || (g.hue - turn >= 17.0f && seedUniform(h) < 0.5f)) {
      turn = -turn;
    }
    g2 = g;
    g2.hue = g.hue + turn;
    if (family == kBlue) {
      g2.top = g.top + turn;
    }
  } else {
    drawFrom(pair[family][seedUniform(h) < 0.5f], h, g2);
    g2.chroma *= choice.chroma;
  }
  paletteRamp(g2, l.ramp2);
  l.ramp2Share = type == 2 ? 0.15f + 0.15f * seedUniform(h) : 0.3f + 0.2f * seedUniform(h);
  l.ramp2Soft = type == 2 ? 0.35f : 0.15f;
  // Where the second palette gives way, as displayed lightness: OKLab 0.7 for a
  // fan, 0.85 for two regions.
  float top = type == 2 ? 0.7f : 0.85f;
  float lt = top * top * top;
  l.ramp2Top = lt <= 0.0031308f ? 12.92f * lt : 1.055f * powf(lt, 1.0f / 2.4f) - 0.055f;
  // The field's value above which that share of the sky lies: evenly spread
  // directions (a Fibonacci sphere), sorted.
  constexpr int n = 512;
  float field[n];
  for (int k = 0; k < n; k++) {
    float z = 1.0f - 2.0f * (k + 0.5f) / n, r = sqrtf(fmaxf(1.0f - z * z, 0.0f));
    float a = 2.39996323f * k, d[3] = {r * cosf(a), z, r * sinf(a)}, v = 0.0f;
    for (int j = 0; j < 3; j++) {
      v += sinf(l.hueWave[j][3] *
                    (d[0] * l.hueWave[j][0] + d[1] * l.hueWave[j][1] + d[2] * l.hueWave[j][2]) +
                l.huePhase[j]);
    }
    field[k] = v / 1.8f;
  }
  std::sort(field, field + n);
  l.ramp2Threshold = field[static_cast<int>((1.0f - l.ramp2Share) * (n - 1))];
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
        l.ramp[i][j] = 1.0f;
      }
    }
    l.grade = 0.0f;
  } else {
    family = drawPalette(seed, choice.family, g);
    g.chroma *= choice.chroma;
    paletteRamp(g, l.ramp);
    l.grade = choice.strength;
  }
  buildSecond(seed, choice, g, family, l);
}

}  // namespace starcanopy
