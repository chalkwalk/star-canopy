// Reading a cubemap by direction. A synthetic sky whose every texel holds the
// direction it looks along lets each mapping be checked against its contract:
// faces and directions invert each other, the sampling is seamless across
// face edges, the cross's faces meet where they should, the equirect looks
// where its formula says, and a rotation moves the sky where it says.

#include "check.h"
#include "sample.h"

#include <cmath>
#include <cstdio>

using namespace starcanopy;

namespace {

void normalize(float v[3]) {
  float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  for (int k = 0; k < 3; k++) {
    v[k] /= l;
  }
}

// Each texel holds its own direction, shifted to be positive.
Cubemap directionSky(int n) {
  Cubemap c;
  c.size = n;
  for (int f = 0; f < 6; f++) {
    c.faces[f].resize(static_cast<size_t>(n) * n * 3);
    for (int j = 0; j < n; j++) {
      for (int i = 0; i < n; i++) {
        float d[3];
        faceDirection(f, (i + 0.5f) / n * 2 - 1, (j + 0.5f) / n * 2 - 1, d);
        normalize(d);
        for (int k = 0; k < 3; k++) {
          c.faces[f][(static_cast<size_t>(j) * n + i) * 3 + k] = d[k] + 2.0f;
        }
      }
    }
  }
  return c;
}

float distance(const float a[3], const float b[3]) {
  return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) +
                   (a[2] - b[2]) * (a[2] - b[2]));
}

}  // namespace

int main() {
  const int n = 32;
  Cubemap sky = directionSky(n);
  const float texel = 2.0f / n;  // a texel's width, roughly, in direction units

  // Face and direction invert each other.
  for (int f = 0; f < 6; f++) {
    float d[3], s, t;
    int g;
    faceDirection(f, 0.3f, -0.7f, d);
    directionToFace(d, g, s, t);
    CHECK(g == f && std::fabs(s - 0.3f) < 1e-6f && std::fabs(t + 0.7f) < 1e-6f);
  }
  // The GL table's faces look where they are named.
  float d[3];
  faceDirection(0, 0, 0, d);
  CHECK(d[0] == 1 && d[1] == 0 && d[2] == 0);
  faceDirection(2, 0, 0, d);
  CHECK(d[1] == 1);
  faceDirection(5, 0, 0, d);
  CHECK(d[2] == -1);
  // A face's first row is its top: on a side face, +y.
  faceDirection(4, 0, -1, d);
  CHECK(d[1] == 1);

  // Sampling returns the direction sampled, everywhere, edges and corners
  // included, to within the bilinear filter's reach.
  float worst = 0.0f;
  for (int k = 0; k < 20000; k++) {
    float v[3] = {std::sin(k * 1.37f) * std::cos(k * 0.61f), std::cos(k * 1.37f),
                  std::sin(k * 1.37f) * std::sin(k * 0.61f)};
    normalize(v);
    float got[3];
    sampleCube(sky, v, got);
    for (int c = 0; c < 3; c++) {
      got[c] -= 2.0f;
    }
    worst = std::fmax(worst, distance(got, v));
  }
  std::printf("worst sampling error %.4f (a texel is about %.4f)\n", worst, texel);
  CHECK(worst < texel);

  // The cross's neighbouring faces meet: across each shared edge the pixels
  // on either side look along nearly the same direction.
  Image x = cross(sky);
  CHECK(x.width == 4 * n && x.height == 3 * n);
  auto at = [&](int px, int py) { return &x.rgb[(static_cast<size_t>(py) * x.width + px) * 3]; };
  float seam = 0.0f;
  for (int j = 0; j < n; j++) {
    int y = n + j;
    seam = std::fmax(seam, distance(at(n - 1, y), at(n, y)));          // -x | +z
    seam = std::fmax(seam, distance(at(2 * n - 1, y), at(2 * n, y)));  // +z | +x
    seam = std::fmax(seam, distance(at(3 * n - 1, y), at(3 * n, y)));  // +x | -z
    int xx = n + j;
    seam = std::fmax(seam, distance(at(xx, n - 1), at(xx, n)));          // +y over +z
    seam = std::fmax(seam, distance(at(xx, 2 * n - 1), at(xx, 2 * n)));  // +z over -y
  }
  std::printf("worst step across the cross's seams %.4f\n", seam);
  CHECK(seam < 1.5f * texel);

  // The equirect looks where its formula says: its middle along +z, a
  // quarter to the right along +x, its top row up.
  Image map = equirect(sky, 4 * n);
  auto mapAt = [&](int px, int py) {
    const float* p = &map.rgb[(static_cast<size_t>(py) * map.width + px) * 3];
    static float v[3];
    for (int k = 0; k < 3; k++) {
      v[k] = p[k] - 2.0f;
    }
    return v;
  };
  const float* m = mapAt(map.width / 2, map.height / 2);
  CHECK(m[2] > 0.99f);
  m = mapAt(3 * map.width / 4, map.height / 2);
  CHECK(m[0] > 0.99f);
  m = mapAt(0, 0);
  CHECK(m[1] > 0.99f);

  // Rotation: the identity is an exact copy; a turn moves what lay along v to
  // r v; the angles turn the way they are documented to.
  float r[9];
  rotation(0, 0, 0, r);
  CHECK(isIdentity(r));
  Cubemap same = rotate(sky, r);
  CHECK(same.faces == sky.faces);

  rotation(90, 0, 0, r);  // yaw: +z toward +x
  CHECK(std::fabs(r[0 * 3 + 2] - 1.0f) < 1e-6f);  // r * +z = +x
  rotation(0, 90, 0, r);  // pitch: +z toward +y
  CHECK(std::fabs(r[1 * 3 + 2] - 1.0f) < 1e-6f);
  rotation(0, 0, 90, r);  // roll: +x toward +y
  CHECK(std::fabs(r[1 * 3 + 0] - 1.0f) < 1e-6f);

  rotation(30, -20, 10, r);
  Cubemap turned = rotate(sky, r);
  float moved = 0.0f;
  for (int k = 0; k < 2000; k++) {
    float v[3] = {std::sin(k * 2.1f), std::cos(k * 0.9f), std::sin(k * 0.3f + 1.0f)};
    normalize(v);
    float rv[3] = {r[0] * v[0] + r[1] * v[1] + r[2] * v[2], r[3] * v[0] + r[4] * v[1] + r[5] * v[2],
                   r[6] * v[0] + r[7] * v[1] + r[8] * v[2]};
    float got[3];
    sampleCube(turned, rv, got);  // the rotated sky along r v ...
    for (int c = 0; c < 3; c++) {
      got[c] -= 2.0f;
    }
    moved = std::fmax(moved, distance(got, v));  // ... is the original's along v
  }
  std::printf("worst rotation error %.4f\n", moved);
  CHECK(moved < 2.0f * texel);
  return test::finish();
}
