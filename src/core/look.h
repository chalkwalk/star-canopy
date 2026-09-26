#pragma once

namespace starcanopy {

// Everything the bake shades with that is not the scene's geometry. Made from
// the dials by buildSky() (settings.h); the shaders' uniforms of the same names
// say what each does.
struct Look {
  // The field; see field.glsl.
  float foldScale, outerSharpness, holeScale, cavityDensity;
  float detailScale, detailGain, erosion, filament, hardness;
  float contrast;
  float dustAmount, dustScale;

  // The light.
  float density, sigma;
  float lineColor[3][3];  // [O III], H-alpha, [S II]
  float lineStrength[3];
  float oxygenThreshold;
  float dustAlbedo[3];
  float anisotropy, reflection;
  float reddening[3];
  float rimShadow;
  float ionOpacity, dustOpacity;

  // The march.
  float stepFrac;
  int maxSteps;
  int lightRes, lightSteps;
  float exposure;

};

}  // namespace starcanopy
