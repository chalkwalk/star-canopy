#include "bake.h"

#include "noise.h"

#include <cmath>
#include <cstring>

namespace starcanopy {

namespace {

constexpr int kAllClusters = kMaxBubbles * kMaxClusters;


}  // namespace

Baker::Baker() {
  std::string error;
  bool built = true;
  built = built && light_.build({"field.glsl", "light.shader"}, {"f_FragColor"}, error);
  built = built && bake_.build({"field.glsl", "bake.shader"}, {"f_FragColor"}, error);
  if (!built) {
    buildError_ = error;
  }

  std::vector<uint8_t> noise = noiseVolume(kNoiseSize);
  glGenTextures(1, &noise_);
  glBindTexture(GL_TEXTURE_3D, noise_);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, kNoiseSize, kNoiseSize, kNoiseSize, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, noise.data());
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glBindTexture(GL_TEXTURE_3D, 0);

  glGenFramebuffers(1, &framebuffer_);
  // Core profile draws need a bound vertex array, even when the vertex shader
  // makes its positions up out of gl_VertexID.
  glGenVertexArrays(1, &emptyVertexArray_);
}

Baker::~Baker() {
  glDeleteTextures(1, &noise_);
  glDeleteTextures(1, &lightTexture_);
  glDeleteVertexArrays(1, &emptyVertexArray_);
  glDeleteFramebuffers(1, &framebuffer_);
}

bool Baker::ok(std::string& error) const {
  error = buildError_;
  return buildError_.empty();
}

// The field's uniforms, which every program that evaluates the gas carries.
void Baker::uploadField(const Program& p, const Scene& s, const Look& look) {
  float sphere[kMaxBubbles][4] = {}, shape[kMaxBubbles][4] = {};
  float cluster[kAllClusters][4] = {}, rot[kMaxBubbles][9] = {};
  for (int i = 0; i < s.bubbleCount; i++) {
    const Bubble& b = s.bubble[i];
    std::memcpy(sphere[i], b.center, sizeof(b.center));
    sphere[i][3] = b.radius;
    shape[i][0] = b.thickness;
    shape[i][1] = b.fold;
    shape[i][2] = b.keep;
    shape[i][3] = b.noiseOffset;
    for (int c = 0; c < kMaxClusters; c++) {
      float* slot = cluster[i * kMaxClusters + c];
      std::memcpy(slot, b.cluster[c].pos, sizeof(b.cluster[c].pos));
      slot[3] = b.cluster[c].luminosity;
    }
    std::memcpy(rot[i], b.rot, sizeof(b.rot));
  }
  glUniform4fv(p.uniform("u_BubbleSphere"), kMaxBubbles, &sphere[0][0]);
  glUniform4fv(p.uniform("u_BubbleShape"), kMaxBubbles, &shape[0][0]);
  glUniform4fv(p.uniform("u_Cluster"), kAllClusters, &cluster[0][0]);
  // rot is row major, so GL is asked to transpose it into its column major mat3.
  glUniformMatrix3fv(p.uniform("u_BubbleRot"), kMaxBubbles, GL_TRUE, &rot[0][0]);
  glUniform1f(p.uniform("u_FoldScale"), look.foldScale);
  glUniform1f(p.uniform("u_OuterSharpness"), look.outerSharpness);
  glUniform1f(p.uniform("u_HoleScale"), look.holeScale);
  glUniform1f(p.uniform("u_CavityDensity"), look.cavityDensity);
  glUniform1f(p.uniform("u_DetailScale"), look.detailScale);
  glUniform1f(p.uniform("u_DetailGain"), look.detailGain);
  glUniform1f(p.uniform("u_Erosion"), look.erosion);
  glUniform1f(p.uniform("u_Filament"), look.filament);
  glUniform1f(p.uniform("u_Hardness"), look.hardness);
  glUniform1f(p.uniform("u_Contrast"), look.contrast);
  glUniform1f(p.uniform("u_DustAmount"), look.dustAmount);
  glUniform1f(p.uniform("u_DustScale"), look.dustScale);
  glUniform1f(p.uniform("u_DustOpacity"), look.dustOpacity);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, noise_);
  glUniform1i(p.uniform("u_Noise"), 0);
}

void Baker::bakeLight(const Scene& s, const Look& look) {
  int res = look.lightRes;
  if (!light_.id() || s.bubbleCount == 0) {
    return;
  }
  if (!lightTexture_ || lightRes_ != res || lightSlabs_ != s.bubbleCount) {
    if (!lightTexture_) {
      glGenTextures(1, &lightTexture_);
    }
    glBindTexture(GL_TEXTURE_3D, lightTexture_);
    // One channel per cluster, and one slab per bubble stacked along z. GLSL
    // 1.50 cannot index an array of samplers with a loop variable, so eight
    // bubbles cannot have eight textures; one tall texture and a clamp per slab
    // does the same job.
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, res, res, res * s.bubbleCount, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);
    lightRes_ = res;
    lightSlabs_ = s.bubbleCount;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glUseProgram(light_.id());
  uploadField(light_, s, look);
  glUniform1f(light_.uniform("u_Res"), static_cast<float>(res));
  glUniform1i(light_.uniform("u_LightSteps"), look.lightSteps);
  // The light volume carries the density dial too, so it and the bake's own
  // extinction are the same gas, and the ionising opacity, since what it holds
  // is how much ultraviolet is left.
  glUniform1f(light_.uniform("u_Sigma"), look.sigma * look.density * look.ionOpacity);
  glViewport(0, 0, res, res);
  glBindVertexArray(emptyVertexArray_);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  for (int b = 0; b < s.bubbleCount; b++) {
    glUniform1i(light_.uniform("u_Bubble"), b);
    glUniform1f(light_.uniform("u_Bound"), bubbleBound(s.bubble[b]));
    for (int z = 0; z < res; z++) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lightTexture_, 0, b * res + z);
      glUniform1f(light_.uniform("u_Slice"), (static_cast<float>(z) + 0.5f) / static_cast<float>(res));
      // A slice in four strips, each its own job: a thick mass lit by clusters
      // of a size is many times the work of a shell lit by points, and one draw
      // of a whole slice ran long enough for the driver to decide the GPU had
      // hung and reset it.
      glEnable(GL_SCISSOR_TEST);
      for (int t = 0; t < 4; t++) {
        glScissor(0, t * res / 4, res, (t + 1) * res / 4 - t * res / 4);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFlush();
      }
      glDisable(GL_SCISSOR_TEST);
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glFinish();
}

void Baker::begin(CubemapTarget& target, const Scene& s, const Look& look) {
  target_ = &target;
  scene_ = s;
  look_ = look;
  int supersample = 1;
  int size = target.size() * supersample;
  if (size != marchSize_) {
    glBindTexture(GL_TEXTURE_2D, 0);
    marchSize_ = size;
  }
  tiles_ = cubemapTiles(size);
  tilesPerFace_ = static_cast<int>(tiles_.size() / 6);
  next_ = 0;
}

void Baker::uploadBake() {
  const Program& p = bake_;
  const Look& look = look_;
  float bound[kMaxBubbles] = {};
  uploadField(p, scene_, look);
  for (int i = 0; i < scene_.bubbleCount; i++) {
    bound[i] = bubbleBound(scene_.bubble[i]);
  }
  glUniform1i(p.uniform("u_BubbleCount"), scene_.bubbleCount);
  glUniform1fv(p.uniform("u_BubbleBound"), kMaxBubbles, bound);
  glUniform1f(p.uniform("u_FaceSize"), static_cast<float>(marchSize_));
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_3D, lightTexture_);
  glUniform1i(p.uniform("u_Light"), 1);
  glUniform1f(p.uniform("u_LightRes"), static_cast<float>(lightRes_));
  glUniform1f(p.uniform("u_LightSlabs"), static_cast<float>(lightSlabs_));
  glUniform1f(p.uniform("u_Density"), look.density);
  glUniform1f(p.uniform("u_Sigma"), look.sigma);
  glUniform1f(p.uniform("u_StepFrac"), look.stepFrac);
  glUniform1i(p.uniform("u_MaxSteps"), look.maxSteps);
  glUniform3fv(p.uniform("u_LineColor"), 3, &look.lineColor[0][0]);
  glUniform3fv(p.uniform("u_LineStrength"), 1, look.lineStrength);
  glUniform1f(p.uniform("u_OxygenThreshold"), look.oxygenThreshold);
  glUniform3fv(p.uniform("u_DustAlbedo"), 1, look.dustAlbedo);
  glUniform1f(p.uniform("u_Anisotropy"), look.anisotropy);
  glUniform1f(p.uniform("u_Reflection"), look.reflection);
  glUniform1f(p.uniform("u_RimShadow"), look.rimShadow);
  glUniform1f(p.uniform("u_IonOpacity"), look.ionOpacity);
  glUniform3fv(p.uniform("u_Reddening"), 1, look.reddening);
  glUniform1f(p.uniform("u_Exposure"), look.exposure);
}

void Baker::attachMarch() {
  const Tile& tile = tiles_[next_];
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X + tile.face, target_->texture(), 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, marchSize_, marchSize_);
}

bool Baker::step() {
  if (!target_ || !bake_.id() || next_ >= tiles_.size()) {
    return false;
  }
  const Tile& tile = tiles_[next_];
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  // Everything is set up again for every tile: a step may resume after an
  // interface has drawn a frame and changed it all, and a few hundred bytes of
  // uniforms is nothing beside a tile's march.
  glUseProgram(bake_.id());
  uploadBake();
  glUniform1i(bake_.uniform("u_Face"), tile.face);
  attachMarch();
  glEnable(GL_SCISSOR_TEST);
  // The triangle covers the whole face, with the scissor cutting it down to the
  // tile, so gl_FragCoord is the face's own texel coordinate.
  glScissor(tile.x, tile.y, tile.width, tile.height);
  glBindVertexArray(emptyVertexArray_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisable(GL_SCISSOR_TEST);
  next_++;
  // The face is complete, and with it the depths its stars need. Denoised
  // first, so the filter never softens a star.
  if (next_ % static_cast<size_t>(tilesPerFace_) == 0) {
  }
  // Wait for the tile, so the driver never holds a queue of them that together
  // run long enough to trip its watchdog.
  glFinish();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);
  return next_ < tiles_.size();
}

float Baker::progress() const {
  return tiles_.empty() ? 1.0f : static_cast<float>(next_) / static_cast<float>(tiles_.size());
}

}  // namespace starcanopy
