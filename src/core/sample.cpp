#include "sample.h"

#include <cmath>

namespace starcanopy {

namespace {

constexpr double kPi = 3.14159265358979323846;

int clampIndex(int i, int n) {
  return i < 0 ? 0 : (i >= n ? n - 1 : i);
}

// The texel of face f nearest to (s, t), each -1..1.
const float* nearestTexel(const Cubemap& c, int face, float s, float t) {
  int n = c.size;
  int i = clampIndex(static_cast<int>(std::floor((s * 0.5f + 0.5f) * n)), n);
  int j = clampIndex(static_cast<int>(std::floor((t * 0.5f + 0.5f) * n)), n);
  return &c.faces[face][(static_cast<size_t>(j) * n + i) * 3];
}

// Texel (i, j) of face f, where i or j may lie one past the face's edge: then
// it is the texel of the neighbouring face that the same point of the cube's
// surface falls on.
const float* texel(const Cubemap& c, int face, int i, int j) {
  int n = c.size;
  if (i >= 0 && i < n && j >= 0 && j < n) {
    return &c.faces[face][(static_cast<size_t>(j) * n + i) * 3];
  }
  float d[3], s, t;
  int f;
  faceDirection(face, (i + 0.5f) / n * 2.0f - 1.0f, (j + 0.5f) / n * 2.0f - 1.0f, d);
  directionToFace(d, f, s, t);
  return nearestTexel(c, f, s, t);
}

}  // namespace

void faceDirection(int face, float s, float t, float out[3]) {
  // The bake shader's face_direction(), the GL cube map table.
  switch (face) {
    case 0: out[0] = 1.0f; out[1] = -t; out[2] = -s; break;
    case 1: out[0] = -1.0f; out[1] = -t; out[2] = s; break;
    case 2: out[0] = s; out[1] = 1.0f; out[2] = t; break;
    case 3: out[0] = s; out[1] = -1.0f; out[2] = -t; break;
    case 4: out[0] = s; out[1] = -t; out[2] = 1.0f; break;
    default: out[0] = -s; out[1] = -t; out[2] = -1.0f; break;
  }
}

void directionToFace(const float d[3], int& face, float& s, float& t) {
  float ax = std::fabs(d[0]), ay = std::fabs(d[1]), az = std::fabs(d[2]);
  float ma, sc, tc;
  if (ax >= ay && ax >= az) {
    face = d[0] > 0.0f ? 0 : 1;
    ma = ax;
    sc = d[0] > 0.0f ? -d[2] : d[2];
    tc = -d[1];
  } else if (ay >= az) {
    face = d[1] > 0.0f ? 2 : 3;
    ma = ay;
    sc = d[0];
    tc = d[1] > 0.0f ? d[2] : -d[2];
  } else {
    face = d[2] > 0.0f ? 4 : 5;
    ma = az;
    sc = d[2] > 0.0f ? d[0] : -d[0];
    tc = -d[1];
  }
  s = sc / ma;
  t = tc / ma;
}

void sampleCube(const Cubemap& c, const float d[3], float out[3]) {
  int face;
  float s, t;
  directionToFace(d, face, s, t);
  int n = c.size;
  float fs = (s * 0.5f + 0.5f) * n - 0.5f, ft = (t * 0.5f + 0.5f) * n - 0.5f;
  int i = static_cast<int>(std::floor(fs)), j = static_cast<int>(std::floor(ft));
  float wx = fs - i, wy = ft - j;
  const float* a = texel(c, face, i, j);
  const float* b = texel(c, face, i + 1, j);
  const float* e = texel(c, face, i, j + 1);
  const float* g = texel(c, face, i + 1, j + 1);
  for (int k = 0; k < 3; k++) {
    out[k] = (a[k] * (1.0f - wx) + b[k] * wx) * (1.0f - wy) + (e[k] * (1.0f - wx) + g[k] * wx) * wy;
  }
}

Image equirect(const Cubemap& c, int width) {
  Image image;
  image.width = width;
  image.height = width / 2;
  image.rgb.resize(static_cast<size_t>(image.width) * image.height * 3);
  for (int y = 0; y < image.height; y++) {
    double lat = (0.5 - (y + 0.5) / image.height) * kPi;
    for (int x = 0; x < image.width; x++) {
      double lon = ((x + 0.5) / image.width - 0.5) * 2.0 * kPi;
      float d[3] = {static_cast<float>(std::cos(lat) * std::sin(lon)),
                    static_cast<float>(std::sin(lat)),
                    static_cast<float>(std::cos(lat) * std::cos(lon))};
      sampleCube(c, d, &image.rgb[(static_cast<size_t>(y) * image.width + x) * 3]);
    }
  }
  return image;
}

Image cross(const Cubemap& c) {
  // Cell (column, row) of each face, in GL face order.
  static const int kCell[6][2] = {{2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {3, 1}};
  int n = c.size;
  Image image;
  image.width = 4 * n;
  image.height = 3 * n;
  image.rgb.assign(static_cast<size_t>(image.width) * image.height * 3, 0.0f);
  for (int f = 0; f < 6; f++) {
    for (int j = 0; j < n; j++) {
      const float* from = &c.faces[f][static_cast<size_t>(j) * n * 3];
      float* to = &image.rgb[((static_cast<size_t>(kCell[f][1]) * n + j) * image.width +
                              static_cast<size_t>(kCell[f][0]) * n) * 3];
      std::copy(from, from + static_cast<size_t>(n) * 3, to);
    }
  }
  return image;
}

Cubemap rotate(const Cubemap& c, const float r[9]) {
  if (isIdentity(r)) {
    return c;
  }
  Cubemap out;
  out.size = c.size;
  int n = c.size;
  for (int f = 0; f < 6; f++) {
    out.faces[f].resize(static_cast<size_t>(n) * n * 3);
    for (int j = 0; j < n; j++) {
      for (int i = 0; i < n; i++) {
        float d[3], src[3];
        faceDirection(f, (i + 0.5f) / n * 2.0f - 1.0f, (j + 0.5f) / n * 2.0f - 1.0f, d);
        // The sky along r^T d: what lay along v now lies along r v.
        for (int k = 0; k < 3; k++) {
          src[k] = r[0 * 3 + k] * d[0] + r[1 * 3 + k] * d[1] + r[2 * 3 + k] * d[2];
        }
        sampleCube(c, src, &out.faces[f][(static_cast<size_t>(j) * n + i) * 3]);
      }
    }
  }
  return out;
}

void rotation(float yawDegrees, float pitchDegrees, float rollDegrees, float r[9]) {
  double y = yawDegrees * kPi / 180.0, p = pitchDegrees * kPi / 180.0, o = rollDegrees * kPi / 180.0;
  double cy = std::cos(y), sy = std::sin(y), cp = std::cos(p), sp = std::sin(p);
  double co = std::cos(o), so = std::sin(o);
  const double yaw[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy};
  const double pitch[9] = {1, 0, 0, 0, cp, sp, 0, -sp, cp};
  const double roll[9] = {co, -so, 0, so, co, 0, 0, 0, 1};
  double yp[9];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      yp[i * 3 + j] = 0.0;
      for (int k = 0; k < 3; k++) {
        yp[i * 3 + j] += yaw[i * 3 + k] * pitch[k * 3 + j];
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      double v = 0.0;
      for (int k = 0; k < 3; k++) {
        v += yp[i * 3 + k] * roll[k * 3 + j];
      }
      r[i * 3 + j] = static_cast<float>(v);
    }
  }
}

bool isIdentity(const float r[9]) {
  for (int i = 0; i < 9; i++) {
    if (r[i] != (i % 4 == 0 ? 1.0f : 0.0f)) {
      return false;
    }
  }
  return true;
}

}  // namespace starcanopy
