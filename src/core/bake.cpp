#include "bake.h"

#include "noise.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

namespace starcanopy {

namespace {

constexpr int kAllClusters = kMaxBubbles * kMaxClusters;

// The distances at which the march records the optical depth so far, for the
// stars: a geometric series from kTauNearest, and the whole line of sight last.
// Must match NSKY_TAU_LAYERS in the shaders. 0.1 to about 50 sky units covers
// the near wall of a bubble the viewer is inside through to the farthest of
// the distant ones.
constexpr int kTauLayers = 8;
constexpr float kTauNearest = 0.1f;
constexpr float kTauSpacing = 2.8f;

// A star vertex: direction, distance, flux.
constexpr int kStarFloats = 7;

void tauDepths(float depth[kTauLayers - 1]) {
  for (int i = 0; i < kTauLayers - 1; i++) {
    depth[i] = kTauNearest * powf(kTauSpacing, static_cast<float>(i));
  }
}

// The colour the grade gives the stars: its ramp's midtones, where its chroma
// peaks, averaged, at a luminance of 1. That is where the reference skies'
// stars sit -- measured as each star's excess over the sky around it, their
// hues cluster at the palette's and they are at least as colourful as its gas.
// Taken higher up the ramp, where it pales, a blue tint is too weak to outweigh
// what is left of a star's own yellow-white, and the stars come out grey.
void starTint(const Look& look, float tint[3]) {
  for (int j = 0; j < 3; j++) {
    tint[j] = 0.0f;
    for (int i = 3; i <= 5; i++) {
      tint[j] += look.ramp[i][j];
    }
  }
  float lum = 0.2126f * tint[0] + 0.7152f * tint[1] + 0.0722f * tint[2];
  for (int j = 0; j < 3; j++) {
    tint[j] /= lum > 1e-6f ? lum : 1e-6f;
  }
}

// Which side of bubble b is its dense wall, given the clusters' brightness-
// weighted position in its frame; see nsky_blister(). Toward the clusters,
// since the stars are born against the cloud, but also away from the viewer:
// we see the nebula from its open side, as we see Orion. Dense on the viewer's
// side instead puts the wall close all round the eye, and it fills the sky
// however much of the rest is blown out.
//
// From well outside, though, away from the viewer is the wrong way: then we
// look through the opening at the lit inside of the far wall, a bright disc,
// the coin again. A bubble open to one side and seen side on is an arc or a
// crescent, which is what distant nebulae look like; so for those the dense
// side is the clusters' direction with the line of sight taken out of it.
//
// Left unnormalised when there is no side to choose, and a zero length tells
// the shader so.
void blisterAxis(const Bubble& b, float axis[3]) {
  float away[3];
  // The viewer is at the sky's origin; away from it, in the bubble's frame.
  for (int i = 0; i < 3; i++) {
    away[i] = 0.0f;
    for (int j = 0; j < 3; j++) {
      away[i] += b.rot[i * 3 + j] * b.center[j];
    }
  }
  float len = sqrtf(away[0] * away[0] + away[1] * away[1] + away[2] * away[2]);
  float distance = len / b.radius;
  if (len > 1e-6f) {
    for (int i = 0; i < 3; i++) {
      away[i] /= len;
    }
  }
  if (distance > 1.5f) {
    float along = axis[0] * away[0] + axis[1] * away[1] + axis[2] * away[2];
    for (int i = 0; i < 3; i++) {
      axis[i] -= along * away[i];
    }
    return;
  }
  len = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
  if (len > 1e-6f) {
    for (int i = 0; i < 3; i++) {
      axis[i] /= len;
    }
  }
  for (int i = 0; i < 3; i++) {
    axis[i] += away[i];
  }
}

void allocTexture2D(GLuint& texture, int size) {
  if (!texture) {
    glGenTextures(1, &texture);
  }
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size, size, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
  // Nearest: a star reads the depth of the texel it sits in, and a filtered read
  // across the edge of a dark lane would half hide a star that is wholly clear.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

}  // namespace

Baker::Baker() {
  std::string error;
  bool built = true;
  built = built && light_.build({"field.glsl", "light.shader"}, {"f_FragColor"}, error);
  built = built && bake_.build({"field.glsl", "bake.shader"},
                               {"f_FragColor", "f_Tau0", "f_Tau1", "f_Grade"}, error);
  built = built && stars_.build({"stars.shader"}, {"f_FragColor"}, error);
  built = built && galaxy_.build({"field.glsl", "galaxy.glsl", "galaxy.shader"}, {"f_FragColor"},
                                 error);
  built = built && denoise_.build({"denoise.shader"}, {"f_FragColor"}, error);
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
  glGenBuffers(1, &starBuffer_);
  glGenVertexArrays(1, &starVertexArray_);
  // Filter across cube face edges when sampling the galaxy's glow, or every
  // edge of the sky is a seam wherever the band crosses one.
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Baker::~Baker() {
  glDeleteTextures(1, &noise_);
  glDeleteTextures(1, &lightTexture_);
  glDeleteTextures(1, &galaxyTexture_);
  glDeleteTextures(2, tau_);
  glDeleteBuffers(1, &starBuffer_);
  glDeleteVertexArrays(1, &starVertexArray_);
  glDeleteTextures(1, &marched_);
  glDeleteTextures(1, &gradeInfo_);
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
  float axis[kMaxBubbles][3] = {}, form[kMaxBubbles][4] = {};
  float density[kMaxBubbles] = {}, veil[kMaxBubbles][3] = {};
  for (int i = 0; i < s.bubbleCount; i++) {
    const Bubble& b = s.bubble[i];
    std::memcpy(sphere[i], b.center, sizeof(b.center));
    sphere[i][3] = b.radius;
    shape[i][0] = b.stride;
    shape[i][1] = b.fold;
    shape[i][2] = b.keep;
    shape[i][3] = b.noiseOffset;
    for (int c = 0; c < kMaxClusters; c++) {
      float* slot = cluster[i * kMaxClusters + c];
      std::memcpy(slot, b.cluster[c].pos, sizeof(b.cluster[c].pos));
      slot[3] = b.cluster[c].luminosity;
      // The clusters' positions weighted by how bright each is: the dense side
      // is where the light comes from.
      for (int k = 0; k < 3; k++) {
        axis[i][k] += b.cluster[c].pos[k] * b.cluster[c].luminosity;
      }
    }
    blisterAxis(b, axis[i]);
    std::memcpy(form[i], b.squeeze, sizeof(b.squeeze));
    density[i] = b.density;
    std::memcpy(veil[i], b.veil, sizeof(b.veil));
    std::memcpy(rot[i], b.rot, sizeof(b.rot));
  }
  glUniform4fv(p.uniform("u_BubbleSphere"), kMaxBubbles, &sphere[0][0]);
  glUniform4fv(p.uniform("u_BubbleShape"), kMaxBubbles, &shape[0][0]);
  glUniform4fv(p.uniform("u_Cluster"), kAllClusters, &cluster[0][0]);
  // rot is row major, so GL is asked to transpose it into its column major mat3.
  glUniformMatrix3fv(p.uniform("u_BubbleRot"), kMaxBubbles, GL_TRUE, &rot[0][0]);
  glUniform1f(p.uniform("u_FoldScale"), look.foldScale);
  glUniform1f(p.uniform("u_HoleScale"), look.holeScale);
  glUniform1f(p.uniform("u_CavityDensity"), look.cavityDensity);
  glUniform3fv(p.uniform("u_BlisterAxis"), kMaxBubbles, &axis[0][0]);
  glUniform1f(p.uniform("u_Blister"), look.blister);
  glUniform4fv(p.uniform("u_BubbleForm"), kMaxBubbles, &form[0][0]);
  glUniform1fv(p.uniform("u_BubbleDensity"), kMaxBubbles, density);
  glUniform3fv(p.uniform("u_BubbleVeil"), kMaxBubbles, &veil[0][0]);
  glUniform1f(p.uniform("u_DistantDensity"), look.distantDensity);
  glUniform1f(p.uniform("u_DistantCavity"), look.distantCavity);
  glUniform1f(p.uniform("u_DistantBlister"), look.distantBlister);
  glUniform1f(p.uniform("u_MassInner"), look.massInner);
  glUniform1f(p.uniform("u_MassLobes"), look.massLobes);
  glUniform1f(p.uniform("u_MassScale"), look.massScale);
  glUniform1f(p.uniform("u_MassDensity"), look.massDensity);
  glUniform1f(p.uniform("u_MassWarp"), look.massWarp);
  glUniform1f(p.uniform("u_ClusterSize"), look.clusterSize);
  glUniform1f(p.uniform("u_MassFine"), look.massFine);
  glUniform1f(p.uniform("u_MassBillow"), look.massBillow);
  glUniform1f(p.uniform("u_MassEdge"), look.massEdge);
  glUniform1f(p.uniform("u_MassEdgePatch"), look.massEdgePatch);
  glUniform1f(p.uniform("u_DetailScale"), look.detailScale);
  glUniform1f(p.uniform("u_DetailGain"), look.detailGain);
  glUniform1f(p.uniform("u_Erosion"), look.erosion);
  glUniform1f(p.uniform("u_Filament"), look.filament);
  glUniform1f(p.uniform("u_Hardness"), look.hardness);
  glUniform1f(p.uniform("u_Contrast"), look.contrast);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, noise_);
  glUniform1i(p.uniform("u_Noise"), 0);
}

void Baker::bakeLight(const Scene& s, const Look& look) {
  int res = look.lightRes;
  if (!light_.id() || s.bubbleCount == 0) {
    return;
  }
  // Everything the light pass reads: the scene, and every look value the
  // field's uniforms or the pass's own come from. The look is copied whole
  // with the values the light never reads set to zero; the bytes are compared,
  // so a difference only a padding byte shows costs a needless bake, never a
  // stale one.
  Look lit{};
  lit.foldScale = look.foldScale;
  lit.holeScale = look.holeScale;
  lit.cavityDensity = look.cavityDensity;
  lit.distantDensity = look.distantDensity;
  lit.distantCavity = look.distantCavity;
  lit.distantBlister = look.distantBlister;
  lit.massInner = look.massInner;
  lit.massLobes = look.massLobes;
  lit.massScale = look.massScale;
  lit.massDensity = look.massDensity;
  lit.massWarp = look.massWarp;
  lit.massFine = look.massFine;
  lit.massBillow = look.massBillow;
  lit.massEdge = look.massEdge;
  lit.massEdgePatch = look.massEdgePatch;
  lit.clusterSize = look.clusterSize;
  lit.blister = look.blister;
  lit.detailScale = look.detailScale;
  lit.detailGain = look.detailGain;
  lit.erosion = look.erosion;
  lit.filament = look.filament;
  lit.hardness = look.hardness;
  lit.contrast = look.contrast;
  lit.density = look.density;
  lit.sigma = look.sigma;
  lit.ionOpacity = look.ionOpacity;
  lit.lightRes = look.lightRes;
  lit.lightSteps = look.lightSteps;
  std::vector<unsigned char> key(sizeof(Scene) + sizeof(Look));
  std::memcpy(key.data(), &s, sizeof(Scene));
  std::memcpy(key.data() + sizeof(Scene), &lit, sizeof(Look));
  if (key == lightKey_) {
    return;
  }
  lightKey_ = std::move(key);
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
  bindAlone();
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
      // of a size is many times the work of a thin sheet lit by points, and one draw
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

Cubemap Baker::readGalaxy() const {
  Cubemap out;
  out.size = galaxyRes_;
  glBindTexture(GL_TEXTURE_CUBE_MAP, galaxyTexture_);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  for (int f = 0; f < 6; f++) {
    out.faces[f].resize(static_cast<size_t>(galaxyRes_) * galaxyRes_ * 3);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB, GL_FLOAT, out.faces[f].data());
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return out;
}

void Baker::measureGalaxy(const Galaxy& g, const float reddening[3], const float band[3],
                          float out[4]) {
  const int res = 32;
  bakeGalaxy(g, reddening, res, band, 1.0f, 0.0f);
  std::vector<std::pair<float, float>> lum;  // luminance, solid angle
  std::vector<float> texels(static_cast<size_t>(res) * res * 4);
  double total = 0.0;
  glBindTexture(GL_TEXTURE_CUBE_MAP, galaxyTexture_);
  for (int f = 0; f < 6; f++) {
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGBA, GL_FLOAT, texels.data());
    for (int y = 0; y < res; y++) {
      for (int x = 0; x < res; x++) {
        const float* t = &texels[(static_cast<size_t>(y) * res + x) * 4];
        float s = (x + 0.5f) / res * 2.0f - 1.0f, u = (y + 0.5f) / res * 2.0f - 1.0f;
        float w = 1.0f / powf(1.0f + s * s + u * u, 1.5f);
        lum.push_back({0.2126f * t[0] + 0.7152f * t[1] + 0.0722f * t[2], w});
        total += w;
      }
    }
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  std::sort(lum.begin(), lum.end());
  const double q[4] = {0.5, 0.9, 0.99, 0.999};
  double acc = 0.0;
  size_t i = 0;
  for (int k = 0; k < 4; k++) {
    while (i + 1 < lum.size() && acc + lum[i].second < q[k] * total) {
      acc += lum[i].second;
      i++;
    }
    out[k] = lum[i].first;
  }
}

void Baker::bakeGalaxy(const Galaxy& g, const float reddening[3], int res, const float band[3],
                       float exposure, float knee, const std::vector<Accent>* accents,
                       float accentScale) {
  galaxyRes_ = res;
  if (!galaxy_.id()) {
    return;
  }
  if (!galaxyTexture_) {
    glGenTextures(1, &galaxyTexture_);
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, galaxyTexture_);
  for (int i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, res, res, 0, GL_RGBA,
                 GL_HALF_FLOAT, nullptr);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  const Program& p = galaxy_;
  bindAlone();
  glUseProgram(p.id());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, noise_);
  glUniform1i(p.uniform("u_Noise"), 0);
  uploadGalaxy(p, g);
  glUniform3fv(p.uniform("u_Reddening"), 1, reddening);
  glUniform1f(p.uniform("u_FaceSize"), static_cast<float>(res));
  glUniform3fv(p.uniform("u_Band"), 1, band);
  glUniform1f(p.uniform("u_GalExposure"), exposure);
  glUniform1f(p.uniform("u_GalKnee"), knee);
  // The brightest few accents' haze; the rest are points only.
  float dir[kMaxAccentGlows][4] = {}, glow[kMaxAccentGlows][4] = {}, shape[kMaxAccentGlows][4] = {};
  float axis[kMaxAccentGlows][4] = {};
  int count = 0;
  if (accents) {
    std::vector<const Accent*> order;
    for (const Accent& a : *accents) {
      if (a.glow[0] + a.glow[1] + a.glow[2] > 0.0f) {
        order.push_back(&a);
      }
    }
    std::sort(order.begin(), order.end(), [](const Accent* x, const Accent* y) {
      return x->glow[1] > y->glow[1];
    });
    for (const Accent* a : order) {
      if (count == kMaxAccentGlows) {
        break;
      }
      std::memcpy(dir[count], a->dir, sizeof(a->dir));
      dir[count][3] = fmaxf(0.5f * a->radius / a->distance, 1e-4f);  // the core, half its extent
      std::memcpy(glow[count], a->glow, sizeof(a->glow));
      glow[count][3] = a->distance;
      // Where in the noise its texture comes from, by its seed; and how
      // strong: full when young, weaker when old.
      shape[count][0] = static_cast<float>(a->seed % 997u) * 0.37f;
      shape[count][1] = static_cast<float>(a->seed % 991u) * 0.53f;
      shape[count][2] = static_cast<float>(a->seed % 983u) * 0.29f;
      shape[count][3] = 1.0f - 0.6f * fminf(fmaxf(a->age, 0.0f), 1.0f);
      // Its shape, as its members' (1, round, where none was set).
      std::memcpy(axis[count], a->axis, sizeof(a->axis));
      axis[count][3] = fmaxf(a->stretch, 1.0f);
      count++;
    }
  }
  glUniform1i(p.uniform("u_AccentCount"), count);
  glUniform4fv(p.uniform("u_AccentDir"), kMaxAccentGlows, &dir[0][0]);
  glUniform4fv(p.uniform("u_AccentGlow"), kMaxAccentGlows, &glow[0][0]);
  glUniform4fv(p.uniform("u_AccentShape"), kMaxAccentGlows, &shape[0][0]);
  glUniform4fv(p.uniform("u_AccentAxis"), kMaxAccentGlows, &axis[0][0]);
  glUniform1f(p.uniform("u_AccentScale"), accentScale);

  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, res, res);
  glBindVertexArray(emptyVertexArray_);
  // A strip of a face at a time, so no one draw runs long enough to worry a
  // watchdog: at export size a face of 256 steps with the dust's octaves is
  // several seconds.
  const int strip = 64;
  glEnable(GL_SCISSOR_TEST);
  for (int i = 0; i < 6; i++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                           galaxyTexture_, 0);
    glUniform1i(p.uniform("u_Face"), i);
    for (int y = 0; y < res; y += strip) {
      glScissor(0, y, res, std::min(strip, res - y));
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glFinish();
    }
  }
  glDisable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void uploadGalaxy(const Program& p, const Galaxy& g) {
  glUniformMatrix3fv(p.uniform("u_GalRot"), 1, GL_TRUE, g.rot);
  glUniform3fv(p.uniform("u_GalObserver"), 1, g.observer);
  glUniform4f(p.uniform("u_GalDisc"), g.scaleLength, g.scaleHeight, g.flareStart, g.flareLength);
  glUniform1f(p.uniform("u_GalEdge"), g.edge);
  glUniform4f(p.uniform("u_GalArms"), g.arms, g.pitchTan, g.armPhase, g.armStrength);
  glUniform2f(p.uniform("u_GalArmShape"), g.armSharpness, g.flocculence);
  glUniform4f(p.uniform("u_GalBar"), g.barAngle, g.barLength, g.barStrength, g.bulgeStrength);
  glUniform3f(p.uniform("u_GalBulge"), g.bulgeRadius, g.bulgeFlattening, g.dustLane);
  glUniform4f(p.uniform("u_GalLens"), g.lensRadius, g.lensStrength, g.innerRing, g.outerRing);
  glUniform4f(p.uniform("u_GalDust"), g.dust, g.dustHeight, g.dustSigma, g.dustRing);
  glUniform3f(p.uniform("u_GalWarp"), g.warp, g.warpStart, g.warpPhase);
  glUniform3f(p.uniform("u_GalWaves"), g.waves, g.waveLength, g.wavePhase);
  float dir[kMaxExternalGalaxies][4] = {}, major[kMaxExternalGalaxies][4] = {};
  float bright[kMaxExternalGalaxies] = {};
  for (int i = 0; i < g.externalCount; i++) {
    std::memcpy(dir[i], g.external[i].dir, sizeof(g.external[i].dir));
    dir[i][3] = g.external[i].radius;
    std::memcpy(major[i], g.external[i].major, sizeof(g.external[i].major));
    major[i][3] = g.external[i].axisRatio;
    bright[i] = g.external[i].brightness;
  }
  glUniform1i(p.uniform("u_ExternalCount"), g.externalCount);
  glUniform4fv(p.uniform("u_ExternalDir"), kMaxExternalGalaxies, &dir[0][0]);
  glUniform4fv(p.uniform("u_ExternalMajor"), kMaxExternalGalaxies, &major[0][0]);
  glUniform1fv(p.uniform("u_ExternalBrightness"), kMaxExternalGalaxies, bright);
}

void Baker::begin(CubemapTarget& target, const Scene& s, const std::vector<Star>& stars,
                  const Look& look) {
  target_ = &target;
  scene_ = s;
  look_ = look;
  int supersample = look.supersample > 1 ? look.supersample : 1;
  int size = target.size() * supersample;
  if (size != marchSize_) {
    allocTexture2D(tau_[0], size);
    allocTexture2D(tau_[1], size);
    allocTexture2D(marched_, size);
    allocTexture2D(gradeInfo_, size);
    glBindTexture(GL_TEXTURE_2D, 0);
    marchSize_ = size;
  }
  // Six vertices a star, every one carrying the whole star: GL 3.3 core has
  // instancing, but at a few tens of thousands of stars the duplication is a
  // few megabytes, once per bake, and keeps the star pass a plain draw.
  std::vector<float> v(stars.size() * 6 * kStarFloats);
  for (size_t i = 0; i < stars.size(); i++) {
    for (int k = 0; k < 6; k++) {
      float* o = &v[(i * 6 + k) * kStarFloats];
      std::memcpy(&o[0], stars[i].dir, sizeof(stars[i].dir));
      o[3] = stars[i].distance;
      std::memcpy(&o[4], stars[i].flux, sizeof(stars[i].flux));
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, starBuffer_);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(float)), v.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  starVertices_ = static_cast<int>(stars.size() * 6);
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
  float depth[kTauLayers - 1];
  tauDepths(depth);
  glUniform1fv(p.uniform("u_TauDepth"), kTauLayers - 1, depth);
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
  glUniform1f(p.uniform("u_IonOpacity"), look.ionOpacity);
  glUniform3fv(p.uniform("u_Reddening"), 1, look.reddening);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_CUBE_MAP, galaxyTexture_);
  glUniform1i(p.uniform("u_GalaxySky"), 3);
  glUniform1f(p.uniform("u_GalaxyGlow"), galaxyTexture_ ? look.galaxyGlow : 0.0f);
  glUniform1f(p.uniform("u_Exposure"), look.exposure);
  glUniform1f(p.uniform("u_Fill"), look.fill);
  glUniform1f(p.uniform("u_FillShadow"), look.fillShadow);
  glUniform1f(p.uniform("u_Graze"), look.graze);
  glUniform3fv(p.uniform("u_Haze"), 1, look.haze);
}

// The framebuffer, with only colour attachment 0 left to be attached. GL
// clips drawing to the smallest of a framebuffer's attachments whether it is
// drawn to or not, so the march's depth and grade targets, left attached from
// the last bake, cut every pass into anything larger down to their size: a
// galaxy of 512 a face, baked after a march at 256, filled a quarter of each
// face and left the rest undefined.
void Baker::bindAlone() {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  for (int i = 1; i < 4; i++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
  }
}

void Baker::attachMarch() {
  // The march goes to a scratch texture, not the face: it is denoised into the
  // face once the whole face is done. See denoiseFace().
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, marched_, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tau_[0], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, tau_[1], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gradeInfo_, 0);
  static const GLenum buffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                    GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
  glDrawBuffers(4, buffers);
  glViewport(0, 0, marchSize_, marchSize_);
}

// The marched face, box averaged down by the supersampling and then through the
// bilateral filter, into the cubemap's face. Leaves the face attached as the
// only colour target, which is what the star pass wants next.
//
// The supersampling marches at the higher resolution and averages here, not
// several rays in each fragment, though the rays are the same. Measured at
// 2048 a face, two by two: 15 s this way, 25 s the other -- the whole march
// inside a loop holds far more in registers, fewer fragments fit on the GPU at
// once, and it hides its memory latency worse. Many short fragments beat a few
// long ones.
void Baker::denoiseFace(int face) {
  const Program& p = denoise_;
  const Look& look = look_;
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         target_->texture(), 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, target_->size(), target_->size());
  glUseProgram(p.id());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, marched_);
  glUniform1i(p.uniform("u_Marched"), 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gradeInfo_);
  glUniform1i(p.uniform("u_GradeInfo"), 1);
  glUniform1f(p.uniform("u_GradeGalaxy"), look.gradeGalaxy);
  glActiveTexture(GL_TEXTURE0);
  glUniform1f(p.uniform("u_Strength"), look.denoise);
  glUniform1i(p.uniform("u_Supersample"), marchSize_ / target_->size());
  glUniform3fv(p.uniform("u_Ramp"), kRampStops, &look.ramp[0][0]);
  glUniform1f(p.uniform("u_Grade"), look.grade);
  glUniform1f(p.uniform("u_Shoulder"), look.shoulder);
  glUniform1f(p.uniform("u_DisplayGain"), kDisplayGain);
  glUniform1i(p.uniform("u_Face"), face);
  glUniform4fv(p.uniform("u_HueWave"), 3, &look.hueWave[0][0]);
  glUniform3fv(p.uniform("u_HuePhase"), 1, look.huePhase);
  glUniform3fv(p.uniform("u_Ramp2"), kRampStops, &look.ramp2[0][0]);
  glUniform1f(p.uniform("u_Ramp2Share"), look.ramp2Share);
  glUniform1f(p.uniform("u_Ramp2Threshold"), look.ramp2Threshold);
  glUniform1f(p.uniform("u_Ramp2Soft"), look.ramp2Soft);
  glUniform1f(p.uniform("u_Ramp2Top"), look.ramp2Top);
  glBindVertexArray(emptyVertexArray_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

// Every star into face f, over the march's result for that face, reading back
// the depths the march recorded for it.
void Baker::drawStars(int face) {
  const Program& p = stars_;
  const Look& look = look_;
  if (!starVertices_) {
    return;
  }
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, target_->size(), target_->size());
  glUseProgram(p.id());
  glUniform3fv(p.uniform("u_Reddening"), 1, look.reddening);
  float depth[kTauLayers - 1];
  tauDepths(depth);
  glUniform1fv(p.uniform("u_TauDepth"), kTauLayers - 1, depth);
  glUniform1i(p.uniform("u_Face"), face);
  glUniform1f(p.uniform("u_FaceSize"), static_cast<float>(target_->size()));
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, tau_[0]);
  glUniform1i(p.uniform("u_Tau0"), 1);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, tau_[1]);
  glUniform1i(p.uniform("u_Tau1"), 2);
  // Exposure too, so that one dial brightens the whole sky together.
  glUniform1f(p.uniform("u_Brightness"), look.starBrightness * look.exposure * kStarFluxUnit);
  glUniform1f(p.uniform("u_Cutoff"), 0.002f);
  float tint[3];
  starTint(look, tint);
  glUniform3fv(p.uniform("u_StarTint"), 1, tint);
  glUniform1f(p.uniform("u_StarGrade"), look.starGrade);
  glUniform1f(p.uniform("u_HaloAngle"), look.starHaloDegrees * 3.14159265358979323846f / 180.0f);
  glUniform1f(p.uniform("u_Halo"), look.starHalo);
  glUniform1f(p.uniform("u_Spike"), look.starSpike);
  glUniform1f(p.uniform("u_SpikeFlux"),
              look.starSpikeFlux * look.starBrightness * look.exposure * kStarFluxUnit);

  glEnable(GL_BLEND);
  // Added onto the colour; the alpha, the transmittance, is left as it was.
  glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
  glBindVertexArray(starVertexArray_);
  glBindBuffer(GL_ARRAY_BUFFER, starBuffer_);
  auto attribute = [&](const char* name, int size, int offset) {
    GLint loc = glGetAttribLocation(p.id(), name);
    if (loc < 0) {
      return;
    }
    glEnableVertexAttribArray(static_cast<GLuint>(loc));
    glVertexAttribPointer(static_cast<GLuint>(loc), size, GL_FLOAT, GL_FALSE,
                          kStarFloats * sizeof(float),
                          reinterpret_cast<void*>(static_cast<size_t>(offset) * sizeof(float)));
  };
  attribute("a_Dir", 3, 0);
  attribute("a_Distance", 1, 3);
  attribute("a_Flux", 3, 4);
  glDrawArrays(GL_TRIANGLES, 0, starVertices_);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
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
    denoiseFace(tile.face);
    drawStars(tile.face);
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
