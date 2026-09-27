#include "measure.h"

#include "writers.h"

#include <algorithm>
#include <cmath>

namespace starcanopy {

namespace {

// The solid angle a texel at (s, t) of a face covers, up to a constant.
float weight(int n, int i, int j) {
  float s = (i + 0.5f) / n * 2.0f - 1.0f, t = (j + 0.5f) / n * 2.0f - 1.0f;
  float r = 1.0f + s * s + t * t;
  return 1.0f / (r * std::sqrt(r));
}

float srgbDecode(float v) {
  return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
}

// A linear radiance colour, displayed, then into OKLab.
void displayedOklab(const float* rgb, float lab[3]) {
  float c[3];
  for (int k = 0; k < 3; k++) {
    c[k] = srgbDecode(displayed(rgb[k]));
  }
  float l = std::cbrt(0.4122214708f * c[0] + 0.5363325363f * c[1] + 0.0514459929f * c[2]);
  float m = std::cbrt(0.2119034982f * c[0] + 0.6806995451f * c[1] + 0.1073969566f * c[2]);
  float s = std::cbrt(0.0883024619f * c[0] + 0.2817188376f * c[1] + 0.6299787005f * c[2]);
  lab[0] = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
  lab[1] = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
  lab[2] = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
}

}  // namespace

Descriptors describe(const Baked& sky) {
  const Cubemap& c = sky.radiance;
  int n = c.size;
  size_t texels = static_cast<size_t>(n) * n;
  double w = 0, sumL = 0, sumL2 = 0, clear = 0, opaque = 0, chroma = 0, hx = 0, hy = 0, hw = 0;
  double detail = 0, detailW = 0;
  std::vector<std::pair<float, float>> light;  // luminance, weight
  light.reserve(6 * texels);
  std::vector<float> lightness(texels);
  for (int f = 0; f < 6; f++) {
    for (int j = 0; j < n; j++) {
      for (int i = 0; i < n; i++) {
        size_t at = static_cast<size_t>(j) * n + i;
        const float* rgb = &c.faces[f][at * 3];
        float wt = weight(n, i, j), lab[3];
        displayedOklab(rgb, lab);
        lightness[at] = lab[0];
        float ch = std::sqrt(lab[1] * lab[1] + lab[2] * lab[2]);
        w += wt;
        sumL += wt * lab[0];
        sumL2 += wt * lab[0] * lab[0];
        chroma += wt * ch;
        if (ch > 1e-4f) {
          hx += wt * lab[1];
          hy += wt * lab[2];
          hw += wt * ch;
        }
        float tr = sky.transmittance[f][at];
        clear += tr > 0.9f ? wt : 0.0;
        opaque += tr < 0.1f ? wt : 0.0;
        light.emplace_back(0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2], wt);
      }
    }
    for (int j = 1; j < n - 1; j++) {
      for (int i = 1; i < n - 1; i++) {
        size_t at = static_cast<size_t>(j) * n + i;
        float around = 0.25f * (lightness[at - 1] + lightness[at + 1] + lightness[at - n] +
                                lightness[at + n]);
        float wt = weight(n, i, j);
        detail += wt * std::fabs(lightness[at] - around);
        detailW += wt;
      }
    }
  }
  Descriptors d{};
  d.brightness = sumL / w;
  d.contrast = std::sqrt(std::max(0.0, sumL2 / w - d.brightness * d.brightness));
  d.clear = clear / w;
  d.opaque = opaque / w;
  d.chroma = chroma / w;
  d.hue = std::atan2(hy, hx) * 180.0 / 3.14159265358979323846;
  if (d.hue < 0) {
    d.hue += 360.0;
  }
  d.hueSpread = hw > 0 ? 1.0 - std::sqrt(hx * hx + hy * hy) / hw : 0.0;
  d.detail = detail / detailW;
  // The brightest tenth of the sky, by solid angle, and its share of the light.
  std::sort(light.begin(), light.end(),
            [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
              return a.first > b.first;
            });
  double total = 0, top = 0, area = 0;
  for (const auto& [lum, wt] : light) {
    total += lum * wt;
  }
  for (const auto& [lum, wt] : light) {
    if (area >= 0.1 * w) {
      break;
    }
    top += lum * wt;
    area += wt;
  }
  d.brightShare = total > 0 ? top / total : 0.0;
  return d;
}

double change(const Cubemap& a, const Cubemap& b) {
  int n = a.size;
  double sum = 0, w = 0;
  for (int f = 0; f < 6; f++) {
    for (int j = 0; j < n; j++) {
      for (int i = 0; i < n; i++) {
        size_t at = (static_cast<size_t>(j) * n + i) * 3;
        float wt = weight(n, i, j);
        for (int k = 0; k < 3; k++) {
          sum += wt * std::fabs(displayed(a.faces[f][at + k]) - displayed(b.faces[f][at + k]));
        }
        w += 3 * wt;
      }
    }
  }
  return 255.0 * sum / w;
}

}  // namespace starcanopy
