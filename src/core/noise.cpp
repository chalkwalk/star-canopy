#include "noise.h"

#include <cmath>

namespace starcanopy {

namespace {

const signed char kGradient[12][3] = {
  {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
  {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
  {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1},
};

int gradientIndex(int x, int y, int z, int period, uint32_t seed) {
  x = ((x % period) + period) % period;
  y = ((y % period) + period) % period;
  z = ((z % period) + period) % period;
  uint32_t h = static_cast<uint32_t>(x) * 0x8da6b343u + static_cast<uint32_t>(y) * 0xd8163841u +
               static_cast<uint32_t>(z) * 0xcb1ab31fu + seed * 0x165667b1u;
  h ^= h >> 15;
  h *= 0x2c1b3c6du;
  h ^= h >> 12;
  h *= 0x297a2d39u;
  h ^= h >> 15;
  return static_cast<int>(h % 12u);
}

float dotGradient(int gx, int gy, int gz, int period, uint32_t seed, float dx, float dy,
                  float dz) {
  const signed char* g = kGradient[gradientIndex(gx, gy, gz, period, seed)];
  return static_cast<float>(g[0]) * dx + static_cast<float>(g[1]) * dy +
         static_cast<float>(g[2]) * dz;
}

float fade(float t) {
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

}  // namespace

float periodicNoise(float x, float y, float z, int period, uint32_t seed) {
  int xi = static_cast<int>(floorf(x));
  int yi = static_cast<int>(floorf(y));
  int zi = static_cast<int>(floorf(z));
  float xf = x - static_cast<float>(xi);
  float yf = y - static_cast<float>(yi);
  float zf = z - static_cast<float>(zi);
  float u = fade(xf), v = fade(yf), w = fade(zf);
  if (period < 1) {
    period = 1;
  }
  float x00 = lerp(dotGradient(xi, yi, zi, period, seed, xf, yf, zf),
                   dotGradient(xi + 1, yi, zi, period, seed, xf - 1.0f, yf, zf), u);
  float x10 = lerp(dotGradient(xi, yi + 1, zi, period, seed, xf, yf - 1.0f, zf),
                   dotGradient(xi + 1, yi + 1, zi, period, seed, xf - 1.0f, yf - 1.0f, zf), u);
  float x01 = lerp(dotGradient(xi, yi, zi + 1, period, seed, xf, yf, zf - 1.0f),
                   dotGradient(xi + 1, yi, zi + 1, period, seed, xf - 1.0f, yf, zf - 1.0f), u);
  float x11 = lerp(dotGradient(xi, yi + 1, zi + 1, period, seed, xf, yf - 1.0f, zf - 1.0f),
                   dotGradient(xi + 1, yi + 1, zi + 1, period, seed, xf - 1.0f, yf - 1.0f,
                               zf - 1.0f),
                   u);
  return lerp(lerp(x00, x10, v), lerp(x01, x11, v), w);
}

std::vector<uint8_t> noiseVolume(int size) {
  const float cells = static_cast<float>(kNoisePeriod);
  std::vector<uint8_t> out(static_cast<size_t>(size) * size * size * 4);
  for (int z = 0; z < size; z++) {
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        // Texel centres in lattice units, so the last texel and the first are
        // neighbours across the wrap exactly as any other two are.
        float fx = (static_cast<float>(x) + 0.5f) * cells / static_cast<float>(size);
        float fy = (static_cast<float>(y) + 0.5f) * cells / static_cast<float>(size);
        float fz = (static_cast<float>(z) + 0.5f) * cells / static_cast<float>(size);
        uint8_t* p = &out[((static_cast<size_t>(z) * size + y) * size + x) * 4];
        for (int c = 0; c < 4; c++) {
          float n = periodicNoise(fx, fy, fz, kNoisePeriod, 7919u * static_cast<uint32_t>(c + 1));
          float v = n * 0.5f + 0.5f;
          v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
          p[c] = static_cast<uint8_t>(v * 255.0f + 0.5f);
        }
      }
    }
  }
  return out;
}

}  // namespace starcanopy
