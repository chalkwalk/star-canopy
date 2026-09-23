#pragma once

#include "cubemap_target.h"
#include "look.h"
#include "scene.h"
#include "shader.h"
#include "tiles.h"

#include <string>
#include <vector>

namespace starcanopy {

// Turning a scene into a cubemap, on the GPU.
//
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

  // Start baking into the target, whose size is the face's.
  void begin(CubemapTarget& target, const Scene& s, const Look& look);

  // Marches one tile, and finishes a face when its last tile is done. Returns
  // whether any tiles remain.
  bool step();

  float progress() const;

private:
  void uploadField(const Program& p, const Scene& s, const Look& look);
  void uploadBake();
  void attachMarch();

  Program bake_;
  std::string buildError_;
  GLuint noise_ = 0, framebuffer_ = 0, emptyVertexArray_ = 0;
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
