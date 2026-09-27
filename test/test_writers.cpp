// The writers: OpenEXR reads back as written, to the half; the KTX2 file's
// header, descriptor and data are what the format says; the PNGs are PNGs of
// the right size, and the 8-bit derivation follows the display curve.
// (External readers check these too: OpenEXR's own library and
// KTX-Software's `ktx validate`, run by hand on the files cli_bake writes.)

#include "check.h"
#include "half.h"
#include "outputs.h"
#include "writers.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace starcanopy;

namespace {

std::vector<uint8_t> slurp(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

uint32_t u32(const std::vector<uint8_t>& b, size_t at) {
  return b[at] | b[at + 1] << 8 | b[at + 2] << 16 | static_cast<uint32_t>(b[at + 3]) << 24;
}

uint64_t u64(const std::vector<uint8_t>& b, size_t at) {
  return u32(b, at) | static_cast<uint64_t>(u32(b, at + 4)) << 32;
}

Cubemap testSky(int n) {
  Cubemap c;
  c.size = n;
  for (int f = 0; f < 6; f++) {
    for (int i = 0; i < n * n * 3; i++) {
      // Halves, so every value survives exactly; HDR, up to thousands.
      float v = std::pow(2.0f, static_cast<float>((f * 7 + i) % 23) - 11.0f) * (1.0f + (i % 5) / 8.0f);
      c.faces[f].push_back(floatFromHalf(halfFromFloat(v)));
    }
  }
  return c;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: test_writers SCRATCH_DIR\n");
    return 2;
  }
  std::string dir = argv[1], error;
  const int n = 16;
  Cubemap sky = testSky(n);

  // OpenEXR: every face back exactly, orientation included.
  OutputRequest request;
  request.directory = dir;
  request.name = "writers";
  request.formats = outputFormats();
  std::vector<std::string> written;
  CHECK(writeOutputs(sky, request, written, error));
  if (!error.empty()) {
    std::printf("%s\n", error.c_str());
  }
  CHECK(written.size() == 6 + 1 + 1 + 6 + 1 + 1);
  for (int f = 0; f < 6; f++) {
    Image back;
    CHECK(readExr(dir + "/writers_" + faceName(f) + ".exr", back, error));
    CHECK(back.width == n && back.height == n);
    CHECK(back.rgb == sky.faces[f]);
  }
  Image map;
  CHECK(readExr(dir + "/writers_equirect.exr", map, error));
  CHECK(map.width == 4 * n && map.height == 2 * n);

  // KTX2, field by field.
  std::vector<uint8_t> k = slurp(dir + "/writers.ktx2");
  static const uint8_t id[12] = {0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n'};
  CHECK(k.size() > 80 && std::memcmp(k.data(), id, 12) == 0);
  CHECK(u32(k, 12) == 97);  // VK_FORMAT_R16G16B16A16_SFLOAT
  CHECK(u32(k, 16) == 2 && u32(k, 20) == n && u32(k, 24) == n && u32(k, 28) == 0);
  CHECK(u32(k, 32) == 0 && u32(k, 36) == 6 && u32(k, 40) == 1 && u32(k, 44) == 0);
  uint32_t dfd = u32(k, 48), dfdLength = u32(k, 52);
  CHECK(dfdLength == 92 && u32(k, dfd) == 92);
  CHECK(k[dfd + 12] == 1 && k[dfd + 13] == 1 && k[dfd + 14] == 1);  // RGBSDA, BT.709, linear
  uint64_t data = u64(k, 80), length = u64(k, 88);
  CHECK(data % 8 == 0 && length == 6ull * n * n * 8 && data + length == k.size());
  bool same = true;
  for (int f = 0; f < 6; f++) {
    for (int i = 0; i < n * n; i++) {
      size_t at = data + (static_cast<size_t>(f) * n * n + i) * 8;
      for (int c = 0; c < 4; c++) {
        uint16_t h = static_cast<uint16_t>(k[at + 2 * c] | k[at + 2 * c + 1] << 8);
        float want = c < 3 ? sky.faces[f][static_cast<size_t>(i) * 3 + c] : 1.0f;
        same = same && floatFromHalf(h) == want;
      }
    }
  }
  CHECK(same);

  // PNG: a PNG, of the size of the thing it shows.
  auto pngSize = [&](const std::string& path, uint32_t& w, uint32_t& h) {
    std::vector<uint8_t> p = slurp(path);
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (p.size() < 24 || std::memcmp(p.data(), sig, 8) != 0) {
      return false;
    }
    w = static_cast<uint32_t>(p[16]) << 24 | p[17] << 16 | p[18] << 8 | p[19];
    h = static_cast<uint32_t>(p[20]) << 24 | p[21] << 16 | p[22] << 8 | p[23];
    return true;
  };
  uint32_t w = 0, h = 0;
  CHECK(pngSize(dir + "/writers_px.png", w, h) && w == n && h == n);
  CHECK(pngSize(dir + "/writers_cross.png", w, h) && w == 4 * n && h == 3 * n);
  CHECK(pngSize(dir + "/writers_equirect.png", w, h) && w == 4 * n && h == 2 * n);

  // The 8-bit derivation: black is black, bright saturates, the dither never
  // moves a value by more than one step from the curve, and the mean over the
  // dither is the curve.
  CHECK(display8(0.0f, 3, 4, 0) == 0);
  CHECK(display8(1000.0f, 3, 4, 0) == 255);
  double sum = 0.0;
  int far = 0;
  for (int i = 0; i < 4096; i++) {
    int v = display8(0.05f, i % 64, i / 64, i % 3);
    far += std::fabs(v - displayed(0.05f) * 255.0f) > 1.5f;
    sum += v;
  }
  CHECK(far == 0);
  CHECK(std::fabs(sum / 4096 - displayed(0.05f) * 255.0) < 0.1);

  CHECK(!writeOutputs(sky, OutputRequest{dir, "x", {"gif"}, 0}, written, error));
  return test::finish();
}
