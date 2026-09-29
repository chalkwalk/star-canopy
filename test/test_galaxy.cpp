// The galaxy's density has two twins -- galaxyDensity() here, which places the
// stars, and gal_density() in galaxy.glsl, which makes the glow -- and they are
// one specification (PRINCIPLES §13). This holds them to it at points about the
// observer and across the disc. They differ by the noise alone: the shader
// reads it from the 8-bit volume, trilinear, and the CPU computes it exactly.

#include "bake.h"
#include "check.h"
#include "galaxy.h"
#include "gl_context.h"
#include "noise.h"
#include "random.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace starcanopy;

int main() {
  std::string error;
  auto context = GlContext::create(ContextKind::Auto, error);
  if (!context) {
    std::printf("no context: %s\n", error.c_str());
    return 1;
  }
  Program probe;
  if (!probe.build({"field.glsl", "galaxy.glsl", "galaxy_probe.shader"}, {"f_FragColor"}, error)) {
    std::printf("%s\n", error.c_str());
    return 1;
  }

  std::vector<uint8_t> volume = noiseVolume(kNoiseSize);
  GLuint noise, target, framebuffer, vertexArray;
  glGenTextures(1, &noise);
  glBindTexture(GL_TEXTURE_3D, noise);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, kNoiseSize, kNoiseSize, kNoiseSize, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, volume.data());
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  for (GLenum wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R}) {
    glTexParameteri(GL_TEXTURE_3D, wrap, GL_REPEAT);
  }
  const int n = 64;
  glGenTextures(1, &target);
  glBindTexture(GL_TEXTURE_2D, target);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, n, 1, 0, GL_RGBA, GL_FLOAT, nullptr);
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
  glGenVertexArrays(1, &vertexArray);
  glBindVertexArray(vertexArray);

  double worst[3] = {0.0, 0.0, 0.0};
  // At full detail, and at footprints where the dust's finer octaves fade.
  for (float footprint : {0.0f, 0.02f, 0.3f}) {
  for (uint32_t seed : {1u, 7u, 42u}) {
    for (int style = 0; style < kGalaxyStyles; style++) {
      GalaxyParams params;
      params.style = style;
      Galaxy g = generateGalaxy(seed, params);
      // Half about the observer, where the stars are drawn; half across the disc.
      Random rng(seed * 31u + static_cast<uint32_t>(style));
      std::vector<float> points(n * 3);
      for (int i = 0; i < n; i++) {
        float d[3];
        rng.unitVector(d);
        float r = i < n / 2 ? 1.5f * cbrtf(rng.uniform()) : 0.0f;
        for (int k = 0; k < 3; k++) {
          points[i * 3 + k] = i < n / 2 ? g.observer[k] + d[k] * r : 0.0f;
        }
        if (i >= n / 2) {
          float radius = rng.range(0.0f, g.edge), phi = rng.range(0.0f, 6.2831853f);
          points[i * 3 + 0] = radius * cosf(phi);
          points[i * 3 + 1] = radius * sinf(phi);
          points[i * 3 + 2] = rng.range(-0.5f, 0.5f);
        }
      }
      glUseProgram(probe.id());
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, noise);
      glUniform1i(probe.uniform("u_Noise"), 0);
      uploadGalaxy(probe, g);
      glUniform3fv(probe.uniform("u_Points"), n, points.data());
      glUniform1f(probe.uniform("u_Footprint"), footprint);
      glViewport(0, 0, n, 1);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      std::vector<float> gpu(n * 4);
      glReadPixels(0, 0, n, 1, GL_RGBA, GL_FLOAT, gpu.data());

      // Relative to the largest value of each term among the points, so a
      // near-zero term far from the disc does not make a large ratio of noise.
      float scale[3] = {1e-9f, 1e-9f, 1e-9f};
      std::vector<GalaxySample> cpu(n);
      for (int i = 0; i < n; i++) {
        cpu[i] = galaxyDensity(g, &points[i * 3], footprint);
        scale[0] = std::fmax(scale[0], cpu[i].old);
        scale[1] = std::fmax(scale[1], cpu[i].young);
        scale[2] = std::fmax(scale[2], cpu[i].dust);
      }
      for (int i = 0; i < n; i++) {
        const float c[3] = {cpu[i].old, cpu[i].young, cpu[i].dust};
        for (int k = 0; k < 3; k++) {
          worst[k] = std::fmax(worst[k], std::fabs(gpu[i * 4 + k] - c[k]) / scale[k]);
        }
      }
    }
  }
  }
  std::printf("worst difference, relative to each term's largest: old %.4f young %.4f dust %.4f\n",
              worst[0], worst[1], worst[2]);
  CHECK(glGetError() == GL_NO_ERROR);
  // The noise volume's 8 bits and trilinear filtering move a clump factor by a
  // little: measured, at most 0.6% of old light and 1.5% of young on these
  // points. The dust is a lognormal of seven octaves, which sums the rounding
  // of each and multiplies it about four times in the exponent: 5.4% at full
  // detail, under 0.1% once the finest octaves fade. A twin that drifted apart
  // differs by far more.
  CHECK(worst[0] < 0.05);
  CHECK(worst[1] < 0.05);
  CHECK(worst[2] < 0.08);
  return test::finish();
}
