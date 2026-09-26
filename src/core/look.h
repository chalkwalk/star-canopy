#pragma once

namespace starcanopy {

// Stops in the grade's colour ramps; must match RAMP_STOPS in denoise.shader.
constexpr int kRampStops = 8;

// The gain of the display curve the grade places its ramp by: the filmic curve
// the look was judged through, Space Nerds In Space's, where the sky was first
// made, at that game's default tonemapping gain. The grade colours each texel
// by its lightness AS DISPLAYED, so this is part of the look, not of the
// output: an 8-bit derivation may use any tonemap it states (PRINCIPLES §8).
constexpr float kDisplayGain = 1.18f;

// Everything the bake shades with that is not the scene's geometry. Made from
// the dials by buildSky() (settings.h); the shaders' uniforms of the same names
// say what each does.
struct Look {
  // The field; see field.glsl.
  float foldScale, outerSharpness, holeScale, cavityDensity;
  float blister;
  float detailScale, detailGain, erosion, filament, hardness;
  float contrast;
  float pillarDensity, cloudDensity;
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
  float starBrightness, starHalo, starHaloDegrees;
  float starSpike;      // share of a spiked star's light in its spikes
  float starSpikeFlux;  // stars brighter than this get spikes
  float galaxyGlow;

  // The march.
  float stepFrac;
  int maxSteps;
  int lightRes, lightSteps;
  float exposure;

  // The grade: the sky's colour as a ramp along its displayed lightness, from
  // shadows to highlights, kRampStops evenly spaced stops of linear colour of
  // luminance 1, and how far toward it the physical colour goes, 0..1. The
  // same for dust, which the grade moves toward as far as the dust goes in
  // each texel. See denoise.shader.
  float ramp[kRampStops][3];
  float dustRamp[kRampStops][3];
  float grade;
  // A second palette over part of the sky: its ramp; the share of the sky it
  // covers, 0 for none; where a smooth field of three waves -- each a direction
  // and a frequency, with their phases -- passes the threshold, soft either
  // side; and the displayed lightness it gives way to the first by.
  float ramp2[kRampStops][3];
  float ramp2Share, ramp2Threshold, ramp2Soft, ramp2Top;
  float hueWave[3][4], huePhase[3];
  // How far the galaxy's light is graded with the rest; 0 leaves it its own.
  float gradeGalaxy;
  // A faint glow over everything, linear HDR after exposure, so the darkest
  // sky is dim rather than black.
  float haze[3];
  // The brightest channel is eased toward this rather than left to clip; 0 off.
  float shoulder;
  // How far the stars' colours go toward the grade's, 0..1.
  float starGrade;
  float denoise;    // the bilateral filter's tolerance; 0 turns it off
  int supersample;  // rays per texel along each side, 1 or 2
};

}  // namespace starcanopy
