#pragma once

#include "cubemap.h"
#include "image.h"

#include <cstdint>
#include <string>

namespace starcanopy {

// The output formats (DESIGN.md §8). HDR first: OpenEXR and KTX2 carry the
// linear radiance the bake made, unclipped (PRINCIPLES §8). PNG is derived
// from it, never the other way round.

// OpenEXR, scanline, half float R, G and B, ZIP compressed, first row at the
// top. Half is what the bake stores, so nothing is lost.
bool writeExr(const std::string& path, const Image& image, std::string& error);

// Reads one back, for the tests: an EXR's R, G and B as floats.
bool readExr(const std::string& path, Image& image, std::string& error);

// A KTX2 cubemap: VK_FORMAT_R16G16B16A16_SFLOAT, linear, one level, the six
// faces in GL order (+x -x +y -y +z -z), each first row at the top, alpha 1.
// Loads into a GL or Vulkan cubemap as it is.
bool writeKtx2(const std::string& path, const Cubemap& cubemap, std::string& error);

// The 8-bit derivation: each channel through the display curve the look was
// judged through (look.h), which gives display values already encoded for an
// sRGB screen, then to 8 bits with a triangular dither of one step, seeded by
// position so the result depends on nothing but the image. Display-referred:
// for an engine that tonemaps its sky itself, the HDR outputs are the ones to
// use.
bool writePng(const std::string& path, const Image& image, std::string& error);

// The display curve, 0..1, of linear radiance y: the one the grade places its
// ramp by, so that what the grade coloured by its displayed lightness is
// displayed at that lightness.
float displayed(float y);

// The 8-bit value writePng() gives channel value v at pixel (x, y), channel k.
uint8_t display8(float v, int x, int y, int k);

}  // namespace starcanopy
