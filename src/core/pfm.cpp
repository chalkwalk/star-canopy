// StarCanopy -- Portable Float Map, the interim HDR writer.
// Copyright (C) 2026 ChalkWalk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pfm.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

namespace starcanopy {

namespace {

using File = std::unique_ptr<FILE, int (*)(FILE*)>;

bool hostIsLittleEndian() {
  const uint16_t one = 1;
  unsigned char first;
  std::memcpy(&first, &one, 1);
  return first == 1;
}

}  // namespace

bool writePfm(const std::string& path, int width, int height, const std::vector<float>& rgb,
              std::string& error) {
  if (width <= 0 || height <= 0 || rgb.size() != static_cast<size_t>(width) * height * 3) {
    error = path + ": image size does not match its data";
    return false;
  }
  File f(std::fopen(path.c_str(), "wb"), std::fclose);
  if (!f) {
    error = path + ": cannot open for writing";
    return false;
  }
  // A negative scale declares little-endian data; big-endian hosts are not a
  // target, and a positive scale says so honestly if one ever runs this.
  std::fprintf(f.get(), "PF\n%d %d\n%s\n", width, height, hostIsLittleEndian() ? "-1.0" : "1.0");
  if (std::fwrite(rgb.data(), sizeof(float), rgb.size(), f.get()) != rgb.size()) {
    error = path + ": write failed";
    return false;
  }
  return true;
}

bool readPfm(const std::string& path, int& width, int& height, std::vector<float>& rgb,
             std::string& error) {
  File f(std::fopen(path.c_str(), "rb"), std::fclose);
  if (!f) {
    error = path + ": cannot open for reading";
    return false;
  }
  char magic[3] = {};
  float scale = 0.0f;
  if (std::fscanf(f.get(), "%2s %d %d %f", magic, &width, &height, &scale) != 4 ||
      std::strcmp(magic, "PF") != 0 || width <= 0 || height <= 0) {
    error = path + ": not a colour PFM";
    return false;
  }
  if ((scale < 0.0f) != hostIsLittleEndian()) {
    error = path + ": byte order differs from this machine's";
    return false;
  }
  std::fgetc(f.get());  // the single whitespace character that ends the header
  rgb.resize(static_cast<size_t>(width) * height * 3);
  if (std::fread(rgb.data(), sizeof(float), rgb.size(), f.get()) != rgb.size()) {
    error = path + ": truncated";
    return false;
  }
  return true;
}

}  // namespace starcanopy
