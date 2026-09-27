#include "cubemap_target.h"

#include <cstddef>

namespace starcanopy {

CubemapTarget::CubemapTarget(int size) : size_(size) {
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);
  // Half floats, as in the labs: HDR range with room to spare, at half the
  // memory of full floats, which matters at 2048 per face with supersampling.
  for (int face = 0; face < 6; face++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA16F, size, size, 0, GL_RGBA,
                 GL_HALF_FLOAT, nullptr);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &framebuffer_);
  for (int face = 0; face < 6; face++) {
    bindFace(face);
    glClearColor(kUnwrittenTexel, kUnwrittenTexel, kUnwrittenTexel, kUnwrittenTexel);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

CubemapTarget::~CubemapTarget() {
  glDeleteFramebuffers(1, &framebuffer_);
  glDeleteTextures(1, &texture_);
}

void CubemapTarget::bindFace(int face) {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         texture_, 0);
  glViewport(0, 0, size_, size_);
}

Cubemap CubemapTarget::read() const {
  Cubemap cubemap;
  cubemap.size = size_;
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  for (int face = 0; face < 6; face++) {
    cubemap.faces[face].resize(static_cast<size_t>(size_) * size_ * 3);
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, GL_FLOAT,
                  cubemap.faces[face].data());
  }
  return cubemap;
}

}  // namespace starcanopy
