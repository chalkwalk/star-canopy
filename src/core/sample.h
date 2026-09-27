#pragma once

#include "cubemap.h"
#include "image.h"

namespace starcanopy {

// Reading a cubemap by direction, on the CPU, for everything made from a
// baked sky after the bake: the equirectangular map, the cross, a rotated sky.
//
// Directions are cubemap lookup vectors -- the vector a game samples the
// cubemap with -- in the GL convention every engine's cubemap loader follows:
// face f's texel (i, j), row j from the top, holds the direction that
// faceDirection() gives for s = (i + 0.5) / n * 2 - 1, t = (j + 0.5) / n * 2 - 1.
// That is the table the bake shader marches by, so these are the directions
// the sky was made at.

// The unnormalised direction through (s, t), each -1..1, of face f.
void faceDirection(int face, float s, float t, float out[3]);

// The face a direction falls on, and where on it, each -1..1.
void directionToFace(const float d[3], int& face, float& s, float& t);

// The sky along direction d, bilinear, across face edges as well as within
// faces: a texel's neighbour past an edge is found on the face it belongs to.
void sampleCube(const Cubemap& c, const float d[3], float out[3]);

// The whole sky as an equirectangular map, width by width / 2. Pixel (x, y)
// looks along
//   longitude = ((x + 0.5) / width - 0.5) * 360 degrees
//   latitude  = (0.5 - (y + 0.5) / height) * 180 degrees
//   d = (cos lat sin lon, sin lat, cos lat cos lon)
// so the middle of the map looks along +z, +x is a quarter turn to its right,
// and +y is at the top: the same handedness as the face images, whose +z face
// has +x to its right. Engines differ in theirs; the formula is the contract.
Image equirect(const Cubemap& c, int width);

// The six faces as a horizontal cross, 4n by 3n, the layout engines import:
//        +y
//   -x   +z   +x   -z
//        -y
// each face as it is written alone. The empty cells are black.
Image cross(const Cubemap& c);

// The sky turned by the rotation r (row major, orthonormal): the result along
// direction d is the sky along r^T d, so what lay along v now lies along r v.
// The identity copies the cubemap exactly, texel for texel; anything else is
// resampled, bilinear.
Cubemap rotate(const Cubemap& c, const float r[9]);

// A rotation from angles in degrees, r = yaw * pitch * roll: yaw turns the sky
// about +y, pitch about +x, roll about +z. Positive yaw carries +z toward +x
// (seen facing +z, the sky turns to the right); positive pitch carries +z
// toward +y (it tilts up); positive roll carries +x toward +y.
void rotation(float yawDegrees, float pitchDegrees, float rollDegrees, float r[9]);

bool isIdentity(const float r[9]);

}  // namespace starcanopy
