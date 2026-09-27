#pragma once

#include "cubemap.h"

#include <string>
#include <vector>

namespace starcanopy {

// Which files a baked sky is written as, and where. File names are the name
// with a suffix: name_px.exr .. name_nz.exr, name_equirect.exr, name.ktx2,
// name_px.png .. name_nz.png, name_cross.png, name_equirect.png.
struct OutputRequest {
  std::string directory = ".";
  std::string name = "sky";
  // exr-faces, exr-equirect, ktx2, png-faces, png-cross, png-equirect.
  std::vector<std::string> formats = {"exr-faces", "ktx2"};
  int equirectWidth = 0;  // 0: four times the face size, the same texel density
};

// Every format name writeOutputs() knows, for help text and validation.
const std::vector<std::string>& outputFormats();

bool writeOutputs(const Cubemap& sky, const OutputRequest& request,
                  std::vector<std::string>& written, std::string& error);

}  // namespace starcanopy
