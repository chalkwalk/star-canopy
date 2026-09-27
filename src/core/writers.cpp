#include "writers.h"

#include "half.h"
#include "look.h"

#include "stb_image_write.h"
#include "tinyexr.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace starcanopy {

namespace {

using File = std::unique_ptr<FILE, int (*)(FILE*)>;

void put32(std::vector<uint8_t>& out, uint32_t v) {
  for (int i = 0; i < 4; i++) {
    out.push_back(static_cast<uint8_t>(v >> (8 * i)));
  }
}

void put64(std::vector<uint8_t>& out, uint64_t v) {
  for (int i = 0; i < 8; i++) {
    out.push_back(static_cast<uint8_t>(v >> (8 * i)));
  }
}

void set32(std::vector<uint8_t>& out, size_t at, uint32_t v) {
  for (int i = 0; i < 4; i++) {
    out[at + i] = static_cast<uint8_t>(v >> (8 * i));
  }
}

void set64(std::vector<uint8_t>& out, size_t at, uint64_t v) {
  for (int i = 0; i < 8; i++) {
    out[at + i] = static_cast<uint8_t>(v >> (8 * i));
  }
}

void pad(std::vector<uint8_t>& out, size_t alignment) {
  while (out.size() % alignment) {
    out.push_back(0);
  }
}

uint32_t hash(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

}  // namespace

bool writeExr(const std::string& path, const Image& image, std::string& error) {
  size_t n = static_cast<size_t>(image.width) * image.height;
  if (image.width <= 0 || image.height <= 0 || image.rgb.size() != n * 3) {
    error = path + ": image size does not match its data";
    return false;
  }
  // EXR keeps its channels sorted by name, so B, G, R.
  std::vector<float> planes[3];
  for (int c = 0; c < 3; c++) {
    planes[c].resize(n);
    for (size_t i = 0; i < n; i++) {
      planes[c][i] = image.rgb[i * 3 + (2 - c)];
    }
  }
  float* pointers[3] = {planes[0].data(), planes[1].data(), planes[2].data()};

  EXRImage exr;
  InitEXRImage(&exr);
  exr.num_channels = 3;
  exr.images = reinterpret_cast<unsigned char**>(pointers);
  exr.width = image.width;
  exr.height = image.height;

  EXRHeader header;
  InitEXRHeader(&header);
  EXRChannelInfo channels[3];
  int pixelTypes[3], requestedTypes[3];
  const char* names[3] = {"B", "G", "R"};
  for (int c = 0; c < 3; c++) {
    std::memset(&channels[c], 0, sizeof(channels[c]));
    std::strncpy(channels[c].name, names[c], sizeof(channels[c].name) - 1);
    pixelTypes[c] = TINYEXR_PIXELTYPE_FLOAT;
    requestedTypes[c] = TINYEXR_PIXELTYPE_HALF;
  }
  header.num_channels = 3;
  header.channels = channels;
  header.pixel_types = pixelTypes;
  header.requested_pixel_types = requestedTypes;
  header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;

  const char* message = nullptr;
  int result = SaveEXRImageToFile(&exr, &header, path.c_str(), &message);
  if (result != TINYEXR_SUCCESS) {
    error = path + ": " + (message ? message : "could not write");
    FreeEXRErrorMessage(message);
    return false;
  }
  return true;
}

bool readExr(const std::string& path, Image& image, std::string& error) {
  float* rgba = nullptr;
  const char* message = nullptr;
  int width = 0, height = 0;
  if (LoadEXR(&rgba, &width, &height, path.c_str(), &message) != TINYEXR_SUCCESS) {
    error = path + ": " + (message ? message : "could not read");
    FreeEXRErrorMessage(message);
    return false;
  }
  image.width = width;
  image.height = height;
  image.rgb.resize(static_cast<size_t>(width) * height * 3);
  for (size_t i = 0; i < static_cast<size_t>(width) * height; i++) {
    for (int c = 0; c < 3; c++) {
      image.rgb[i * 3 + c] = rgba[i * 4 + c];
    }
  }
  free(rgba);
  return true;
}

bool writeKtx2(const std::string& path, const Cubemap& cubemap, std::string& error) {
  const uint32_t n = static_cast<uint32_t>(cubemap.size);
  const uint64_t faceBytes = static_cast<uint64_t>(n) * n * 8;
  for (const auto& face : cubemap.faces) {
    if (n == 0 || face.size() != static_cast<size_t>(n) * n * 3) {
      error = path + ": cubemap size does not match its data";
      return false;
    }
  }
  std::vector<uint8_t> out;
  // Header: the identifier, then the image's description.
  static const uint8_t kIdentifier[12] = {0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n'};
  out.insert(out.end(), kIdentifier, kIdentifier + 12);
  put32(out, 97);  // VK_FORMAT_R16G16B16A16_SFLOAT
  put32(out, 2);   // typeSize: the size of one component
  put32(out, n);   // pixelWidth
  put32(out, n);   // pixelHeight
  put32(out, 0);   // pixelDepth: not a 3D image
  put32(out, 0);   // layerCount: not an array
  put32(out, 6);   // faceCount: a cubemap
  put32(out, 1);   // levelCount: one level, no mipmaps
  put32(out, 0);   // supercompressionScheme: none
  // Index, filled in once the sections' places are known.
  size_t index = out.size();
  for (int i = 0; i < 4; i++) {
    put32(out, 0);  // dfdByteOffset, dfdByteLength, kvdByteOffset, kvdByteLength
  }
  put64(out, 0);  // sgdByteOffset: no supercompression global data
  put64(out, 0);  // sgdByteLength
  size_t levelIndex = out.size();
  put64(out, 0);           // byteOffset of level 0
  put64(out, 6 * faceBytes);  // byteLength
  put64(out, 6 * faceBytes);  // uncompressedByteLength

  // Data Format Descriptor: one basic block, four 16-bit signed float samples,
  // linear, BT.709 primaries.
  size_t dfd = out.size();
  const uint32_t blockSize = 24 + 16 * 4;
  put32(out, 4 + blockSize);         // dfdTotalSize
  put32(out, 0);                     // vendorId 0 (Khronos), descriptorType 0 (basic)
  put32(out, 2u | (blockSize << 16));  // versionNumber 2, descriptorBlockSize
  out.push_back(1);                  // colorModel: KHR_DF_MODEL_RGBSDA
  out.push_back(1);                  // colorPrimaries: KHR_DF_PRIMARIES_BT709
  out.push_back(1);                  // transferFunction: KHR_DF_TRANSFER_LINEAR
  out.push_back(0);                  // flags: straight alpha
  put32(out, 0);                     // texelBlockDimension 0..3: 1x1x1x1
  out.push_back(8);                  // bytesPlane0: one texel
  for (int i = 1; i < 8; i++) {
    out.push_back(0);
  }
  static const uint8_t kChannel[4] = {0, 1, 2, 15};  // R, G, B, alpha
  for (int i = 0; i < 4; i++) {
    out.push_back(static_cast<uint8_t>(16 * i));  // bitOffset, low byte
    out.push_back(0);                             // bitOffset, high byte
    out.push_back(15);                            // bitLength - 1
    out.push_back(kChannel[i] | 0x80 | 0x40);     // channel, KHR_DF_SAMPLE_DATATYPE_FLOAT | SIGNED
    put32(out, 0);                                // samplePosition 0..3
    put32(out, 0xBF800000u);                      // sampleLower: -1.0f
    put32(out, 0x3F800000u);                      // sampleUpper: 1.0f
  }
  size_t dfdLength = out.size() - dfd;

  // Key/value data: who wrote it.
  size_t kvd = out.size();
  static const char kWriter[] = "KTXwriter\0StarCanopy";
  const uint32_t pairLength = sizeof(kWriter);  // key, NUL, value, NUL
  put32(out, pairLength);
  out.insert(out.end(), kWriter, kWriter + pairLength);
  pad(out, 4);
  size_t kvdLength = out.size() - kvd;

  // The level's data, aligned to the texel's 8 bytes: each face in turn, each
  // row in turn from the top, RGBA half.
  pad(out, 8);
  size_t data = out.size();
  set32(out, index + 0, static_cast<uint32_t>(dfd));
  set32(out, index + 4, static_cast<uint32_t>(dfdLength));
  set32(out, index + 8, static_cast<uint32_t>(kvd));
  set32(out, index + 12, static_cast<uint32_t>(kvdLength));
  set64(out, levelIndex, data);
  out.reserve(out.size() + 6 * faceBytes);
  const uint16_t one = halfFromFloat(1.0f);
  for (const auto& face : cubemap.faces) {
    for (size_t i = 0; i < static_cast<size_t>(n) * n; i++) {
      for (int c = 0; c < 3; c++) {
        uint16_t h = halfFromFloat(face[i * 3 + c]);
        out.push_back(static_cast<uint8_t>(h));
        out.push_back(static_cast<uint8_t>(h >> 8));
      }
      out.push_back(static_cast<uint8_t>(one));
      out.push_back(static_cast<uint8_t>(one >> 8));
    }
  }

  File f(std::fopen(path.c_str(), "wb"), std::fclose);
  if (!f || std::fwrite(out.data(), 1, out.size(), f.get()) != out.size()) {
    error = path + ": could not write";
    return false;
  }
  return true;
}

float displayed(float y) {
  // The filmic curve of denoise.shader's displayed(), which the grade uses.
  float x = std::fmax(y - 0.004f, 0.0f);
  float v = kDisplayGain * (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
  return std::fmin(std::fmax(v, 0.0f), 1.0f);
}

uint8_t display8(float v, int x, int y, int k) {
  uint32_t h = hash(static_cast<uint32_t>(x) * 3u + static_cast<uint32_t>(k) +
                    hash(static_cast<uint32_t>(y) + 0x9e3779b9u));
  float d = static_cast<float>(h & 0xffffu) / 65536.0f - static_cast<float>(h >> 16) / 65536.0f;
  float q = std::floor(displayed(v) * 255.0f + 0.5f + d);
  return static_cast<uint8_t>(std::fmin(std::fmax(q, 0.0f), 255.0f));
}

bool writePng(const std::string& path, const Image& image, std::string& error) {
  size_t n = static_cast<size_t>(image.width) * image.height;
  if (image.width <= 0 || image.height <= 0 || image.rgb.size() != n * 3) {
    error = path + ": image size does not match its data";
    return false;
  }
  std::vector<uint8_t> bytes(n * 3);
  for (int y = 0; y < image.height; y++) {
    for (int x = 0; x < image.width; x++) {
      size_t i = (static_cast<size_t>(y) * image.width + x) * 3;
      for (int k = 0; k < 3; k++) {
        bytes[i + k] = display8(image.rgb[i + k], x, y, k);
      }
    }
  }
  if (!stbi_write_png(path.c_str(), image.width, image.height, 3, bytes.data(), image.width * 3)) {
    error = path + ": could not write";
    return false;
  }
  return true;
}

}  // namespace starcanopy
