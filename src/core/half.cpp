#include "half.h"

#include <cstring>

namespace starcanopy {

uint16_t halfFromFloat(float f) {
  uint32_t x;
  std::memcpy(&x, &f, 4);
  uint32_t sign = (x >> 16) & 0x8000u;
  uint32_t exponent = (x >> 23) & 0xffu;
  uint32_t mantissa = x & 0x7fffffu;
  if (exponent == 0xffu) {
    // Infinity stays infinity; a NaN stays a NaN, quiet.
    return static_cast<uint16_t>(sign | 0x7c00u | (mantissa ? 0x200u | (mantissa >> 13) : 0u));
  }
  int e = static_cast<int>(exponent) - 127 + 15;
  if (e >= 31) {
    return static_cast<uint16_t>(sign | 0x7c00u);  // too large: infinity
  }
  if (e <= 0) {
    // Subnormal, or zero: the implicit bit made explicit and shifted down.
    if (e < -10) {
      return static_cast<uint16_t>(sign);
    }
    uint32_t m = mantissa | 0x800000u;
    int shift = 14 - e;
    uint32_t half = m >> shift;
    uint32_t rest = m & ((1u << shift) - 1u), midpoint = 1u << (shift - 1);
    if (rest > midpoint || (rest == midpoint && (half & 1u))) {
      half++;
    }
    return static_cast<uint16_t>(sign | half);
  }
  uint32_t half = (static_cast<uint32_t>(e) << 10) | (mantissa >> 13);
  uint32_t rest = mantissa & 0x1fffu;
  if (rest > 0x1000u || (rest == 0x1000u && (half & 1u))) {
    half++;  // may carry into the exponent, and to infinity, as it should
  }
  return static_cast<uint16_t>(sign | half);
}

float floatFromHalf(uint16_t h) {
  uint32_t sign = static_cast<uint32_t>(h & 0x8000u) << 16;
  uint32_t exponent = (h >> 10) & 0x1fu;
  uint32_t mantissa = h & 0x3ffu;
  uint32_t x;
  if (exponent == 0) {
    if (mantissa == 0) {
      x = sign;
    } else {
      // Subnormal: normalise it.
      int e = -1;
      do {
        e++;
        mantissa <<= 1;
      } while (!(mantissa & 0x400u));
      x = sign | (static_cast<uint32_t>(127 - 15 - e) << 23) | ((mantissa & 0x3ffu) << 13);
    }
  } else if (exponent == 31) {
    x = sign | 0x7f800000u | (mantissa << 13);
  } else {
    x = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
  }
  float f;
  std::memcpy(&f, &x, 4);
  return f;
}

}  // namespace starcanopy
