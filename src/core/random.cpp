#include "random.h"

#include <cmath>

namespace starcanopy {

namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

float dot3(const float a[3], const float b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void normalize3(float v[3]) {
  float l = sqrtf(dot3(v, v));
  if (l > 0.0f) {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

void Random::unitVector(float v[3]) {
  float z = range(-1.0f, 1.0f);
  float a = range(0.0f, 2.0f * kPi);
  float r = sqrtf(fmaxf(0.0f, 1.0f - z * z));
  v[0] = r * cosf(a);
  v[1] = r * sinf(a);
  v[2] = z;
}

void Random::perpendicular(const float v[3], float out[3]) {
  float d;
  do {
    unitVector(out);
    d = dot3(out, v);
  } while (fabsf(d) > 0.9f);
  out[0] -= d * v[0];
  out[1] -= d * v[1];
  out[2] -= d * v[2];
  normalize3(out);
}

}  // namespace starcanopy
