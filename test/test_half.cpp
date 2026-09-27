// Half floats: every one of the 65536 comes back from float as itself, and
// floats between halves round to the nearest, ties to even.

#include "check.h"
#include "half.h"

#include <cmath>
#include <cstdio>
#include <limits>

using namespace starcanopy;

int main() {
  int wrong = 0;
  for (uint32_t h = 0; h < 65536; h++) {
    float f = floatFromHalf(static_cast<uint16_t>(h));
    bool nan = ((h >> 10) & 0x1f) == 31 && (h & 0x3ff);
    if (nan) {
      wrong += !std::isnan(f) || !std::isnan(floatFromHalf(halfFromFloat(f)));
    } else {
      wrong += halfFromFloat(f) != h;
    }
  }
  if (wrong) {
    std::printf("%d halves do not round trip\n", wrong);
  }
  CHECK(wrong == 0);

  CHECK(floatFromHalf(0x3c00) == 1.0f);
  CHECK(floatFromHalf(0x7bff) == 65504.0f);         // the largest finite half
  CHECK(floatFromHalf(0x0001) == std::ldexp(1.0f, -24));  // the smallest subnormal
  CHECK(halfFromFloat(65520.0f) == 0x7c00);          // rounds up to infinity
  CHECK(halfFromFloat(65519.0f) == 0x7bff);
  CHECK(halfFromFloat(std::numeric_limits<float>::infinity()) == 0x7c00);
  CHECK(halfFromFloat(-0.0f) == 0x8000);
  CHECK(halfFromFloat(std::ldexp(1.0f, -25)) == 0x0000);        // tie, to even: zero
  CHECK(halfFromFloat(std::ldexp(1.5f, -25)) == 0x0001);
  CHECK(halfFromFloat(1.0f + std::ldexp(1.0f, -11)) == 0x3c00);  // tie, to even
  CHECK(halfFromFloat(1.0f + std::ldexp(3.0f, -11)) == 0x3c02);  // tie, to even
  CHECK(halfFromFloat(1.0f + std::ldexp(1.0f, -10)) == 0x3c01);
  return test::finish();
}
