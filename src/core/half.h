#pragma once

#include <cstdint>

namespace starcanopy {

// IEEE 754 binary16, the bake's own storage. A baked sky is read back from a
// half-float texture, so its values are halves already and the conversion
// back is exact; rounding is to nearest, ties to even, for anything else.
uint16_t halfFromFloat(float f);
float floatFromHalf(uint16_t h);

}  // namespace starcanopy
