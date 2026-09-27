#include "render.h"

#include "cubemap_target.h"
#include "sample.h"
#include "sky.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>

namespace starcanopy {

namespace {

// The sidecar: what was made, and where its key light is -- a lookup vector,
// its azimuth and elevation in the equirect's frame, and the way its light
// travels, which is what a scene's directional light is set to.
bool writeSidecar(const std::string& path, const Project& p, const float light[3],
                  const std::vector<std::string>& files, std::string& error) {
  std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "w"), std::fclose);
  if (!f) {
    error = path + ": could not write";
    return false;
  }
  const double degrees = 180.0 / 3.14159265358979323846;
  double azimuth = std::atan2(light[0], light[2]) * degrees;
  double elevation = std::asin(std::fmax(-1.0f, std::fmin(1.0f, light[1]))) * degrees;
  int count = 0;
  const Dial* d = dials(count);
  std::string form;
  for (int i = 0; i < count; i++) {
    if (std::string(d[i].name) == "form") {
      form = dialValue(p.settings, d[i]);
    }
  }
  std::fprintf(f.get(),
               "{\n"
               "  \"generator\": \"StarCanopy\",\n"
               "  \"look\": %d,\n"
               "  \"seed\": %u,\n"
               "  \"style\": \"%s\",\n"
               "  \"size\": %d,\n"
               "  \"orientation\": {\"yaw\": %g, \"pitch\": %g, \"roll\": %g},\n"
               "  \"key_light\": {\n"
               "    \"toward\": [%.6f, %.6f, %.6f],\n"
               "    \"azimuth_degrees\": %.3f,\n"
               "    \"elevation_degrees\": %.3f,\n"
               "    \"light_travels\": [%.6f, %.6f, %.6f]\n"
               "  },\n"
               "  \"conventions\": \"Directions are cubemap lookup vectors, in the GL cube map "
               "convention the faces are written in. Azimuth is measured from +z toward +x, "
               "elevation toward +y, as in the equirectangular map, whose centre looks along +z.\",\n"
               "  \"files\": [",
               p.look, p.settings.seed, form.c_str(), p.size, p.yaw, p.pitch, p.roll, light[0],
               light[1], light[2], azimuth, elevation, -light[0], -light[1], -light[2]);
  for (size_t i = 0; i < files.size(); i++) {
    std::fprintf(f.get(), "%s\"%s\"", i ? ", " : "",
                 std::filesystem::path(files[i]).filename().string().c_str());
  }
  std::fprintf(f.get(), "]\n}\n");
  return true;
}

}  // namespace

bool renderProject(const Project& p, std::vector<std::string>& written, std::string& error) {
  std::error_code ec;
  std::filesystem::create_directories(p.output.directory, ec);
  if (ec) {
    error = p.output.directory + ": " + ec.message();
    return false;
  }
  CubemapTarget target(p.size);
  if (!bakeSky(p.settings, target, error)) {
    return false;
  }
  // Orientation is an output decision, not a regeneration (PRINCIPLES §14):
  // the baked sky is turned, and the key light with it.
  float r[9], light[3], turned[3];
  rotation(p.yaw, p.pitch, p.roll, r);
  Cubemap sky = rotate(target.read(), r);
  keyLight(p.settings, light);
  for (int i = 0; i < 3; i++) {
    turned[i] = r[i * 3 + 0] * light[0] + r[i * 3 + 1] * light[1] + r[i * 3 + 2] * light[2];
  }
  std::vector<std::string> files;
  if (!writeOutputs(sky, p.output, files, error)) {
    return false;
  }
  std::string sidecar = p.output.directory + "/" + p.output.name + ".json";
  if (!writeSidecar(sidecar, p, turned, files, error)) {
    return false;
  }
  files.push_back(sidecar);
  written.insert(written.end(), files.begin(), files.end());
  return true;
}

}  // namespace starcanopy
