// The galaxy's density has two twins -- galaxyDensity() here, which places the
// stars, and gal_density() in galaxy.glsl, which makes the glow -- and they are
// one specification (PRINCIPLES §13). This holds them to it at points about the
// observer and across the disc. They differ by the noise alone: the shader
// reads it from the 8-bit volume, trilinear, and the CPU computes it exactly.

#include "accents.h"
#include "bake.h"
#include "cubemap.h"
#include "check.h"
#include "galaxy.h"
#include "gl_context.h"
#include "noise.h"
#include "random.h"

#include <cmath>
#include <cstdio>
#include <string>
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
  // Where each term's worst was, so a failure says where the twins part; and
  // values that are not finite, which fmax would pass over.
  char where[3][160] = {"", "", ""};
  int nonFinite = 0;
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
          if (!std::isfinite(gpu[i * 4 + k])) {
            if (nonFinite++ < 5) {
              std::printf("not finite: term %d, seed %u style %d footprint %g point %d\n", k,
                          seed, style, footprint, i);
            }
            continue;
          }
          double d = std::fabs(gpu[i * 4 + k] - c[k]) / scale[k];
          if (d > worst[k]) {
            worst[k] = d;
            std::snprintf(where[k], sizeof where[k],
                          "seed %u style %d footprint %g point %d (%.3f %.3f %.3f): gpu %g cpu %g",
                          seed, style, footprint, i, points[i * 3], points[i * 3 + 1],
                          points[i * 3 + 2], gpu[i * 4 + k], c[k]);
          }
        }
      }
    }
  }
  }
  std::printf("worst difference, relative to each term's largest: old %.4f young %.4f dust %.4f\n",
              worst[0], worst[1], worst[2]);
  for (int k = 0; k < 3; k++) {
    std::printf("  %s worst at %s\n", k == 0 ? "old" : k == 1 ? "young" : "dust", where[k]);
  }
  CHECK(nonFinite == 0);
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

  // The observer's path: galactic +1 nearer the centre than the seed's own
  // place, -1 further out, and the own place in the band.
  for (uint32_t seed = 1; seed <= 50; seed++) {
    float r0, h0, rIn, hIn, rOut, hOut;
    observerPlace(seed, 0.0f, r0, h0);
    observerPlace(seed, 1.0f, rIn, hIn);
    observerPlace(seed, -1.0f, rOut, hOut);
    CHECK(r0 >= 2.2f && r0 <= 3.0f && std::fabs(h0) <= 0.03f);
    CHECK(rIn < r0 && std::fabs(hIn) < 1e-6f);
    CHECK(rOut > r0 + 1.0f);  // mostly outward: 0 to 45 degrees
  }
  // An open cluster's haze lies where it is, and nowhere else: one cluster
  // straight along the sky's +z, 0.5 kpc off, against none.
  {
    Baker baker;
    std::string why;
    CHECK(baker.ok(why));
    GalaxyParams gp;
    gp.observerRadius = 2.6f;
    gp.observerHeight = 0.0f;
    Galaxy hg = generateGalaxy(1, gp);
    const float reddening[3] = {0.8f, 1.0f, 1.3f}, band[3] = {0.0f, 0.0f, 0.3f};
    Accent c{};
    c.kind = kOpenCluster;
    c.dir[2] = 1.0f;
    c.distance = 0.5f;
    c.radius = 0.01f;
    c.glow[0] = c.glow[1] = c.glow[2] = 1.0e6f;
    std::vector<Accent> one = {c}, none;
    const int res = 32;
    baker.bakeGalaxy(hg, reddening, res, band, 1.0f, 1.5f, &none, 1.0e-6f);
    Cubemap without = baker.readGalaxy();
    baker.bakeGalaxy(hg, reddening, res, band, 1.0f, 1.5f, &one, 1.0e-6f);
    Cubemap with = baker.readGalaxy();
    // Face +z is GL face 4; its middle texel looks along +z; a corner far off.
    auto at = [&](const Cubemap& m, int face, int x, int y) {
      return m.faces[face][(static_cast<size_t>(y) * res + x) * 3 + 1];
    };
    float centre = at(with, 4, res / 2, res / 2) - at(without, 4, res / 2, res / 2);
    float corner = at(with, 4, 0, 0) - at(without, 4, 0, 0);
    float behind = at(with, 5, res / 2, res / 2) - at(without, 5, res / 2, res / 2);
    std::printf("haze: %.4g at the cluster, %.4g off it, %.4g behind\n", centre, corner, behind);
    CHECK(centre > 0.0f);
    CHECK(std::fabs(corner) < 0.01f * centre && std::fabs(behind) < 1e-6f);

    // Textured, not a smooth halo: round a ring about a large cluster its haze
    // varies, as a young cluster's birth cloud and a reflection nebula's wisps
    // do in photographs -- never a round even glow (the design's addendum).
    const int big = 128;
    Accent wide = c;
    wide.radius = 0.05f;  // a core of 0.05 rad at 0.5 kpc
    std::vector<Accent> w = {wide};
    baker.bakeGalaxy(hg, reddening, big, band, 1.0f, 1.5f, &none, 1.0e-6f);
    Cubemap plain = baker.readGalaxy();
    baker.bakeGalaxy(hg, reddening, big, band, 1.0f, 1.5f, &w, 1.0e-6f);
    Cubemap hazy = baker.readGalaxy();
    double sum = 0.0, sum2 = 0.0;
    const int points = 16;
    for (int i = 0; i < points; i++) {
      float phi = 6.2831853f * i / points;
      // Face +z looks along (s, -t, 1): a direction (x, y, 1) is at s = x, t = -y.
      float x = 0.08f * std::cos(phi), y = 0.08f * std::sin(phi);
      int px = static_cast<int>((x + 1.0f) / 2.0f * big), py = static_cast<int>((-y + 1.0f) / 2.0f * big);
      size_t at = (static_cast<size_t>(py) * big + px) * 3 + 1;
      double v = hazy.faces[4][at] - plain.faces[4][at];
      sum += v;
      sum2 += v * v;
    }
    double mean = sum / points, spread = std::sqrt(std::fmax(sum2 / points - mean * mean, 0.0));
    std::printf("haze round a ring: mean %.4g, varying %.0f%%\n", mean, 100.0 * spread / mean);
    // (Sampling a smooth halo on the texel grid alone gives some 16%.)
    CHECK(mean > 0.0 && spread > 0.4 * mean);

    // The same light at every size (PRINCIPLES §9): a cluster whose core is
    // smaller than a texel was point-sampled, a preview keeping 2-35% of the
    // export's haze, and jumping with where it fell in a texel (the review).
    // Its total -- each texel's value over its solid angle -- must hold.
    Accent small = c;
    small.radius = 0.00125f;  // a core of 0.00125 rad at 0.5 kpc
    for (int k = 0; k < 3; k++) {
      small.glow[k] = 1.0e3f;
    }
    small.dir[0] = 0.003f;  // off a texel's axis
    float nd = std::sqrt(small.dir[0] * small.dir[0] + 1.0f);
    small.dir[0] /= nd;
    small.dir[2] /= nd;
    std::vector<Accent> s1 = {small};
    double totals[4];
    const int sizes[4] = {32, 64, 128, 256};
    for (int i = 0; i < 4; i++) {
      int n = sizes[i];
      baker.bakeGalaxy(hg, reddening, n, band, 1.0f, 1.5f, &none, 1.0e-6f);
      Cubemap off = baker.readGalaxy();
      baker.bakeGalaxy(hg, reddening, n, band, 1.0f, 1.5f, &s1, 1.0e-6f);
      Cubemap on = baker.readGalaxy();
      double total = 0.0;
      for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
          float s = (x + 0.5f) / n * 2.0f - 1.0f, t = (y + 0.5f) / n * 2.0f - 1.0f;
          double omega = (4.0 / (n * n)) / std::pow(1.0 + s * s + t * t, 1.5);
          size_t at = (static_cast<size_t>(y) * n + x) * 3 + 1;
          total += (on.faces[4][at] - off.faces[4][at]) * omega;
        }
      }
      totals[i] = total;
    }
    std::printf("a small cluster's haze, total at 32/64/128/256: %.4g %.4g %.4g %.4g\n", totals[0],
                totals[1], totals[2], totals[3]);
    for (int i = 0; i < 3; i++) {
      CHECK(std::fabs(totals[i] / totals[3] - 1.0) < 0.15);
    }

    // Its haze follows its shape: a cluster stretched along the sky's x has a
    // haze stretched with it -- round under an elongated scatter, it worked
    // against "never round" (the review). Old, so its texture is faint.
    Accent long_ = wide;
    long_.age = 1.0f;
    long_.axis[0] = 1.0f;
    long_.stretch = 2.0f;
    std::vector<Accent> l1 = {long_};
    baker.bakeGalaxy(hg, reddening, big, band, 1.0f, 1.5f, &none, 1.0e-6f);
    Cubemap flat = baker.readGalaxy();
    baker.bakeGalaxy(hg, reddening, big, band, 1.0f, 1.5f, &l1, 1.0e-6f);
    Cubemap stretched = baker.readGalaxy();
    double mxx = 0.0, myy = 0.0;
    for (int y = 0; y < big; y++) {
      for (int x = 0; x < big; x++) {
        float s = (x + 0.5f) / big * 2.0f - 1.0f, t = (y + 0.5f) / big * 2.0f - 1.0f;
        size_t at = (static_cast<size_t>(y) * big + x) * 3 + 1;
        double v = stretched.faces[4][at] - flat.faces[4][at];
        mxx += v * s * s;  // face +z: sky x is s, sky y is -t
        myy += v * t * t;
      }
    }
    std::printf("a stretched cluster's haze: %.2f to 1\n", std::sqrt(mxx / myy));
    CHECK(std::sqrt(mxx / myy) > 1.5);
  }
  return test::finish();
}
