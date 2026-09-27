#include "outputs.h"

#include "sample.h"
#include "writers.h"

#include <algorithm>

namespace starcanopy {

namespace {

bool wants(const OutputRequest& r, const char* format) {
  return std::find(r.formats.begin(), r.formats.end(), format) != r.formats.end();
}

Image faceImage(const Cubemap& sky, int face) {
  Image image;
  image.width = image.height = sky.size;
  image.rgb = sky.faces[face];
  return image;
}

}  // namespace

const std::vector<std::string>& outputFormats() {
  static const std::vector<std::string> formats = {"exr-faces", "exr-equirect", "ktx2",
                                                   "png-faces", "png-cross",    "png-equirect"};
  return formats;
}

bool writeOutputs(const Cubemap& sky, const OutputRequest& request,
                  std::vector<std::string>& written, std::string& error) {
  for (const std::string& f : request.formats) {
    if (std::find(outputFormats().begin(), outputFormats().end(), f) == outputFormats().end()) {
      error = "unknown output format '" + f + "'";
      return false;
    }
  }
  std::string base = request.directory + "/" + request.name;
  auto note = [&](bool ok, const std::string& path) {
    if (ok) {
      written.push_back(path);
    }
    return ok;
  };
  for (int face = 0; face < 6; face++) {
    std::string stem = base + "_" + faceName(face);
    if (wants(request, "exr-faces") &&
        !note(writeExr(stem + ".exr", faceImage(sky, face), error), stem + ".exr")) {
      return false;
    }
    if (wants(request, "png-faces") &&
        !note(writePng(stem + ".png", faceImage(sky, face), error), stem + ".png")) {
      return false;
    }
  }
  if (wants(request, "ktx2") && !note(writeKtx2(base + ".ktx2", sky, error), base + ".ktx2")) {
    return false;
  }
  if (wants(request, "png-cross") &&
      !note(writePng(base + "_cross.png", cross(sky), error), base + "_cross.png")) {
    return false;
  }
  if (wants(request, "exr-equirect") || wants(request, "png-equirect")) {
    Image map = equirect(sky, request.equirectWidth > 0 ? request.equirectWidth : 4 * sky.size);
    if (wants(request, "exr-equirect") &&
        !note(writeExr(base + "_equirect.exr", map, error), base + "_equirect.exr")) {
      return false;
    }
    if (wants(request, "png-equirect") &&
        !note(writePng(base + "_equirect.png", map, error), base + "_equirect.png")) {
      return false;
    }
  }
  return true;
}

}  // namespace starcanopy
