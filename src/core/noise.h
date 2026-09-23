#pragma once

#include <cstdint>
#include <vector>

namespace starcanopy {

// Lattice cells across the noise volume. The field shader must agree: it
// divides by this (NSKY_NOISE_PERIOD).
constexpr int kNoisePeriod = 8;

// Texels per side of the noise volume. 128 at 8 lattice cells is 16 texels a
// cell, enough that trilinear filtering of the stored noise does not show its
// own grid; as RGBA8 it is 8 MB, the cache footprint every march step pays.
constexpr int kNoiseSize = 128;

// Periodic gradient noise (improved Perlin's twelve gradients): repeats exactly
// every `period` units on every axis, roughly -1..1. Wrapping the lattice
// coordinate before hashing is what makes it tile exactly rather than nearly,
// and the shader tiles it across a bubble many times over: a volume that did
// not wrap exactly would lay a grid of seams across the sky.
float periodicNoise(float x, float y, float z, int period, uint32_t seed);

// A tiling volume of SINGLE-octave noise, four independent fields in RGBA8,
// size^3 texels, z-major. Single octave because the bake builds its octaves in
// the shader, one fetch each at its own scale, and so can leave out any octave
// finer than the texel it lands in; a volume with the octaves summed in has
// them whether the texel can hold them or not, and they alias.
//
// Stored as bytes about 128, so the shader recovers it as texel * 2 - 1. The
// field is fixed, not seeded: the sky's seed moves where each bubble reads it,
// which varies the picture as much and costs no regeneration.
std::vector<uint8_t> noiseVolume(int size);

}  // namespace starcanopy
