#include "check.h"
#include "pfm.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace starcanopy;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: test_pfm SCRATCH_DIR\n");
    return 2;
  }
  std::string path = std::string(argv[1]) + "/roundtrip.pfm";

  // Values an HDR sky holds: zero, tiny, above one, and very bright.
  const int width = 3, height = 2;
  std::vector<float> rgb;
  for (int i = 0; i < width * height * 3; i++) {
    rgb.push_back(i * 0.37f - 0.5f);
  }
  rgb[4] = 1e-7f;
  rgb[5] = 65000.0f;

  std::string error;
  CHECK(writePfm(path, width, height, rgb, error));
  int w = 0, h = 0;
  std::vector<float> back;
  CHECK(readPfm(path, w, h, back, error));
  CHECK(w == width && h == height);
  CHECK(back == rgb);

  CHECK(!writePfm(path, width, height + 1, rgb, error));
  CHECK(!readPfm(std::string(argv[1]) + "/does-not-exist.pfm", w, h, back, error));
  std::remove(path.c_str());
  return test::finish();
}
