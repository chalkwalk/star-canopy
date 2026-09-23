// The seeded generator and the noise make the same numbers as in the labs, so
// a seed there is the same sky here. Expected values were computed by the SNIS
// labs' own C (nebula_sky_noise.c, mtwist.c).

#include "check.h"
#include "noise.h"

extern "C" {
#include "mtwist.h"
}

#include <cmath>

using namespace starcanopy;

int main() {
  mtwist_state* mt = mtwist_init(1u);
  CHECK(mtwist_next(mt) == 1791095845u);  // MT19937's first output for seed 1
  CHECK(mtwist_next(mt) == 4282876139u);
  CHECK(mtwist_next(mt) == 3093770124u);
  CHECK(mtwist_next(mt) == 4005303368u);
  mtwist_free(mt);

  // Past the first regeneration of the state, through float and int draws.
  mt = mtwist_init(2654435761u * 7u + 17u);
  for (int i = 0; i < 700; i++) {
    mtwist_next(mt);
  }
  CHECK(mtwist_next(mt) == 3628970790u);
  CHECK(mtwist_float(mt) == 0.419599831f);
  CHECK(mtwist_int(mt, 1000) == 475);
  mtwist_free(mt);

  CHECK(periodicNoise(0.3f, 1.7f, 2.2f, 8, 7919u * 2u) == 0.434966803f);
  CHECK(periodicNoise(5.5f, -3.25f, 7.9f, 8, 7919u * 2u) == -0.0527200699f);
  CHECK(periodicNoise(12.1f, 0.01f, -9.6f, 8, 7919u * 2u) == 0.0963160619f);

  // Exactly periodic, on every axis, at points a period apart exactly (dyadic
  // fractions, so adding the period rounds nothing).
  for (float x : {0.125f, 1.375f, 4.875f}) {
    float a = periodicNoise(x, 2.5f, 3.25f, kNoisePeriod, 7919u);
    CHECK(a == periodicNoise(x + kNoisePeriod, 2.5f, 3.25f, kNoisePeriod, 7919u));
    CHECK(a == periodicNoise(x, 2.5f + kNoisePeriod, 3.25f, kNoisePeriod, 7919u));
    CHECK(a == periodicNoise(x, 2.5f, 3.25f - kNoisePeriod, kNoisePeriod, 7919u));
    CHECK(std::fabs(a) <= 1.0f);
  }

  // The whole volume, byte for byte, at a small size.
  std::vector<uint8_t> v = noiseVolume(16);
  uint32_t h = 2166136261u;
  for (uint8_t b : v) {
    h ^= b;
    h *= 16777619u;
  }
  CHECK(v.size() == 16u * 16u * 16u * 4u);
  CHECK(h == 1006825499u);
  return test::finish();
}
