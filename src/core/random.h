#pragma once

#include <cstdint>

extern "C" {
#include "mtwist.h"
}

namespace starcanopy {

// A seeded stream, and the few ways the model draws from one. Every draw's
// arithmetic is exactly the labs', so a seed makes the same scene here as
// there: change nothing here without meaning to change every sky.
class Random {
public:
  explicit Random(uint32_t seed) : state_(mtwist_init(seed)) {}
  ~Random() { mtwist_free(state_); }
  Random(const Random&) = delete;
  Random& operator=(const Random&) = delete;

  float uniform() { return mtwist_float(state_); }  // 0..1
  float range(float lo, float hi) { return lo + (hi - lo) * uniform(); }
  int below(int n) { return mtwist_int(state_, n); }

  // Uniform on the sphere.
  void unitVector(float v[3]);
  // A unit vector perpendicular to the unit vector v, at random.
  void perpendicular(const float v[3], float out[3]);

private:
  mtwist_state* state_;
};

float dot3(const float a[3], const float b[3]);
void normalize3(float v[3]);

}  // namespace starcanopy
