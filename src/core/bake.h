#pragma once

#include "cubemap_target.h"
#include "galaxy.h"
#include "look.h"
#include "scene.h"
#include "shader.h"
#include "stars.h"
#include "tiles.h"

#include <string>
#include <vector>

namespace starcanopy {

// The galaxy's uniforms, as galaxy.glsl declares them, into a program built
// with it. Shared by the glow pass and the test that holds the shader's density
// to galaxyDensity()'s.
void uploadGalaxy(const Program& p, const Galaxy& g);

// Turning a scene into a cubemap, on the GPU.
//
// The galaxy's glow is baked first, apart from the nebula and small, and is
// sampled by every bake after.
// The light volume is built once per scene and look, in strips.
// The faces are then marched a tile at a time, so that a bake of many seconds
// can be spread across an interface's frames, and so that no single draw runs
// long enough for the driver's watchdog to decide the GPU has hung and reset
// it -- which on the machine the model was developed on takes the desktop with
// it.
//
// Needs a current GL context for its whole life.
class Baker {
public:
  Baker();
  ~Baker();
  Baker(const Baker&) = delete;
  Baker& operator=(const Baker&) = delete;

  // False, with the reason, if the shaders would not build.
  bool ok(std::string& error) const;

  // The galaxy's glow as seen from its observer, res texels a face. Every bake
  // after samples it; it need only be redone when the galaxy changes.
  void bakeGalaxy(const Galaxy& g, const float reddening[3], int res);

  // The light volume for this scene and look. Before any begin() whose scene or
  // look differs from the last one lit.
  void bakeLight(const Scene& s, const Look& look);

  // Start baking into the target, whose size is the face's.
  void begin(CubemapTarget& target, const Scene& s, const std::vector<Star>& stars,
             const Look& look);

  // Marches one tile, and finishes a face when its last tile is done. Returns
  // whether any tiles remain.
  bool step();

  float progress() const;

private:
  void uploadField(const Program& p, const Scene& s, const Look& look);
  void uploadBake();
  void attachMarch();
  void denoiseFace(int face);
  void drawStars(int face);

  Program light_;
  Program bake_;
  Program stars_, galaxy_;
  Program denoise_;
  std::string buildError_;
  GLuint noise_ = 0, framebuffer_ = 0, emptyVertexArray_ = 0;
  GLuint lightTexture_ = 0;
  int lightRes_ = 0, lightSlabs_ = 0;
  // What the light volume was last baked from, so an unchanged one is not
  // baked again: most dials never reach it, and at small sizes it is most of
  // a bake. See bakeLight().
  std::vector<unsigned char> lightKey_;
  GLuint galaxyTexture_ = 0;
  GLuint tau_[2] = {0, 0};
  GLuint starBuffer_ = 0, starVertexArray_ = 0;
  int starVertices_ = 0;
  // Where a face is marched before it is denoised into the cubemap.
  GLuint marched_ = 0;
  // What the grade needs of each marched texel: how much of its view dust
  // took, and the galaxy's light in it.
  GLuint gradeInfo_ = 0;
  int marchSize_ = 0;

  // The bake in progress.
  CubemapTarget* target_ = nullptr;
  Scene scene_{};
  Look look_{};
  std::vector<Tile> tiles_;
  size_t next_ = 0;
  int tilesPerFace_ = 0;
};

}  // namespace starcanopy
