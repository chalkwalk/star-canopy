#pragma once

#include <cstdint>

namespace starcanopy {

// The galaxy the sky is seen from inside, as a 3D density, so the sky is what
// it would be from where the observer actually sits rather than a stripe
// painted round it.
//
// Seen from inside, a galaxy is not a uniform band. Its disc is exponential in
// radius and flares -- thickens -- outward; young stars and dust are thinner
// still and gather in the spiral arms; there may be a bar and there is a bulge;
// the outer disc is warped, up on one side and down on the other, and rippled
// by bending waves. Every one of those shows from inside: the arms as bright
// knots where the line of sight runs along one, the dust as a rift down the
// band, the warp as a band that is not a great circle, and the observer's own
// position as where the light is. Near the centre the bulge fills the sky; out
// at the edge nearly all the light is on one side.
//
// Units are kiloparsecs in the galaxy's own frame: centre at the origin, disc
// in the xy plane. The same density is evaluated in galaxy.glsl for the glow
// and here for the stars: one specification, two twins, tested against each
// other (PRINCIPLES §13).

// A galaxy's type. Spirals are one family of several: a lenticular is a disc
// and a bulge with no arms and little dust -- a legitimate galaxy, and what the
// spirals looked like while their arms were too weak to be seen. Ellipticals,
// with no disc, and irregulars, with no plane, are still to come.
enum GalaxyStyle {
  kBarredSpiral,  // a bar, and two or four arms off its ends
  kGrandDesign,   // two strong, continuous arms; no bar to speak of
  kFlocculent,    // many short, broken arm fragments
  kLenticular,    // a disc and a bulge, no arms, old stars, little dust
  kGalaxyStyles
};

// The type a seed draws, weighted as skies want them -- a spiral mostly, a
// lenticular now and then -- from a stream of its own.
int galaxyStyleFor(uint32_t seed);

constexpr int kMaxExternalGalaxies = 8;

struct GalaxyParams {
  int style = kBarredSpiral;    // a GalaxyStyle, or -1 for the seed's
  // The Sun is at about three scale lengths; 3.5 is out toward the edge, where
  // the galaxy's light is decidedly on one side of the sky.
  float observerRadius = 3.5f;  // from the centre, in disc scale lengths
  // Above the midplane, kpc. The Sun is some 0.02 above it, and so sees more
  // of the disc below than above -- a fifth more stars, in the model -- which
  // a sky nobody chose that for should not have; the seed's choice of place
  // is to come (ROADMAP.md, The observer's place).
  float observerHeight = 0.0f;
  float dust = 1.0f;             // multiplies the dust
  float warp = 1.0f;             // multiplies the warp
  float waves = 1.0f;            // multiplies the bending waves
  int externalGalaxies = 4;      // other galaxies, far off
};

struct ExternalGalaxy {
  float dir[3];
  float major[3];     // the long axis, perpendicular to dir
  float radius;       // angular, radians
  float axisRatio;    // minor over major
  float brightness;
};

struct Galaxy {
  float rot[9];       // sky direction -> galaxy frame, row major
  float observer[3];  // kpc, galaxy frame
  float scaleLength;  // of the disc, kpc
  float scaleHeight;  // of the old disc at the observer's radius...
  float flareStart, flareLength;  // ...growing as exp((R - start) / length) beyond
  float edge;                     // where the disc is truncated, kpc
  float arms, pitchTan, armPhase, armStrength;
  float armSharpness, flocculence;
  float barAngle, barLength, barStrength, bulgeStrength;
  float bulgeRadius;      // kpc: 1 in a spiral, twice that in a lenticular
  float bulgeFlattening;  // its width over its depth
  // A lenticular's lens -- its radius, kpc, 0 for none, and its light -- and
  // its stellar rings at the lens's inner edge and twice that.
  float lensRadius, lensStrength, innerRing, outerRing;
  float dustRing;         // kpc: its dust in rings there and 1.7 out; 0, not
  float dust, dustHeight;
  // The spread of the dust's lognormal: how clumped it is, from a soft haze of
  // lanes to dense clouds with clear windows between. By seed, so skies differ
  // in it and the dust is not always at its most dramatic.
  float dustSigma;
  float warp, warpStart, warpPhase;
  float waves, waveLength, wavePhase;
  int externalCount;
  ExternalGalaxy external[kMaxExternalGalaxies];
};

struct GalaxySample {
  float old;    // old disc, bar and bulge: warm light
  float young;  // the arms' young stars: blue light
  float dust;   // extinction per kpc
};

Galaxy generateGalaxy(uint32_t seed, const GalaxyParams& p);

// The density at p, kpc in the galaxy frame. Must match gal_density() in
// galaxy.glsl. footprint: the size of the sample, kpc, below which the dust's
// detail fades out; 0, all of it.
GalaxySample galaxyDensity(const Galaxy& g, const float p[3], float footprint = 0.0f);

// A sky direction into the galaxy frame.
void galaxyToFrame(const Galaxy& g, const float sky[3], float out[3]);

// The optical depth of the galaxy's dust between the observer and a point
// offset from it, kpc in the galaxy frame: what dims and reddens the stars, and
// what lies in front of a distant nebula.
float dustDepth(const Galaxy& g, const float offset[3]);

}  // namespace starcanopy
