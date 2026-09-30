// The look, pinned (DESIGN.md §10). A few skies are baked small and their
// statistics compared with the ones recorded for this look version, so a
// change that alters any sky's pixels fails here -- and is then either a bug,
// or a new look version made on purpose (PRINCIPLES §7).
//
// Statistics, not a hash: the promise across GPUs is the same sky, not the
// same bits, and drivers round differently. The tolerances are set from the
// Radeon the look was made on against Mesa's llvmpipe, a CPU renderer and as
// different a GPU as there is. Measured, the two agree to 0.2% in the means,
// 0.7% in the median and 1.8% in the brightest tenth of a percent, where a
// few texels decide it; so 1.5% for the means and the body of the
// distribution, 4% for its tail. A 2% change in brightness fails.
//
//   test_look          compare with the recorded statistics
//   test_look --print  print the statistics, to record a new look version

#include "check.h"
#include "cubemap_target.h"
#include "gl_context.h"
#include "project.h"
#include "sky.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace starcanopy;

namespace {

constexpr int kSize = 48;
constexpr int kStats = 7;
const char* const kStatNames[kStats] = {"mean r", "mean g", "mean b", "p50 lum",
                                        "p90 lum", "p99 lum", "p99.9 lum"};

struct Reference {
  uint32_t seed;
  double stat[kStats];
};

// Look 10, recorded on the Radeon (radeonsi) at 48 a face with the default
// dials. A look version's table stays for as long as it is rendered; look 1's
// went when look 2 retired the shell, and each since when the next
// replaced it.
const Reference kLook10[] = {
    {3, {0.0169018, 0.0262293, 0.023523, 0.0146845, 0.0435694, 0.164821, 0.474421}},
    {7, {0.0216155, 0.0248979, 0.0255096, 0.0155798, 0.0463166, 0.146598, 0.412637}},
    {12, {0.017994, 0.0201082, 0.0232233, 0.0134354, 0.0351794, 0.128677, 0.381494}},
};

void measure(const Cubemap& c, double out[kStats]) {
  std::vector<float> lum;
  double sum[3] = {0, 0, 0};
  for (const auto& face : c.faces) {
    for (size_t i = 0; i < face.size(); i += 3) {
      for (int k = 0; k < 3; k++) {
        sum[k] += face[i + k];
      }
      lum.push_back(0.2126f * face[i] + 0.7152f * face[i + 1] + 0.0722f * face[i + 2]);
    }
  }
  std::sort(lum.begin(), lum.end());
  for (int k = 0; k < 3; k++) {
    out[k] = sum[k] / static_cast<double>(lum.size());
  }
  const double q[4] = {0.5, 0.9, 0.99, 0.999};
  for (int k = 0; k < 4; k++) {
    out[3 + k] = lum[static_cast<size_t>(q[k] * (lum.size() - 1))];
  }
}

}  // namespace

int main(int argc, char** argv) {
  bool print = argc > 1 && std::strcmp(argv[1], "--print") == 0;
  std::string error;
  auto context = GlContext::create(ContextKind::Auto, error);
  if (!context) {
    std::printf("no context: %s\n", error.c_str());
    return 1;
  }
  std::printf("context: %s\n", context->description().c_str());
  CHECK(kLookVersion == 10);  // a new look version needs its own table here

  double worst = 0.0;
  for (const Reference& ref : kLook10) {
    Settings s;
    s.seed = ref.seed;
    CubemapTarget target(kSize);
    CHECK(bakeSky(s, target, error));
    double got[kStats];
    measure(target.read(), got);
    if (print) {
      std::printf("    {%u, {", ref.seed);
      for (int k = 0; k < kStats; k++) {
        std::printf("%s%.6g", k ? ", " : "", got[k]);
      }
      std::printf("}},\n");
      continue;
    }
    for (int k = 0; k < kStats; k++) {
      double off = std::fabs(got[k] - ref.stat[k]) / std::fmax(std::fabs(ref.stat[k]), 1e-6);
      double tolerance = k < 5 ? 0.015 : 0.04;
      worst = std::fmax(worst, off / tolerance);
      if (off > tolerance) {
        std::printf("seed %u: %s is %.6g, look %d recorded %.6g (%.1f%% off)\n", ref.seed,
                    kStatNames[k], got[k], kLookVersion, ref.stat[k], 100.0 * off);
      }
      CHECK(off <= tolerance);
    }
  }
  if (!print) {
    std::printf("worst difference from look %d's statistics: %.0f%% of its tolerance\n", kLookVersion,
                100.0 * worst);
  }
  return test::finish();
}
