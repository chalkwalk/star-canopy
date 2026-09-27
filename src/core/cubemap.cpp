#include "cubemap.h"

namespace starcanopy {

const char* faceName(int face) {
  static const char* const kNames[6] = {"px", "nx", "py", "ny", "pz", "nz"};
  return face >= 0 && face < 6 ? kNames[face] : "?";
}

}  // namespace starcanopy
