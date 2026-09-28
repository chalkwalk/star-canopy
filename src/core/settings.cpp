#include "settings.h"

#include "palette.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace starcanopy {

namespace {

const char* const kLineColorNames[] = {"natural", "hubble-sho", "hoo"};
const char* const kGradeNames[] = {"physical", "auto"};
const char* const kHueTypeNames[] = {"auto", "single", "fan", "split"};
const char* const kFamilyNames[] = {"auto", "warm", "green", "teal", "blue"};
const char* const kDustStyleNames[] = {"lanes", "vein", "physical"};
const char* const kFormNames[] = {"shell", "mass"};
const char* const kOnOffNames[] = {"off", "on"};
const char* const kGalaxyStyleNames[] = {"barred-spiral", "grand-design", "flocculent"};

// Line colours per palette: [O III], H-alpha, [S II].
//
//   natural     what the eye would see: [O III] teal at 501 nm, H-alpha deep red
//               with a little H-beta blue in it, [S II] a darker red
//   hubble-sho  the Hubble mapping, [S II] to red, H-alpha to green, [O III] to
//               blue, which is why the Pillars of Creation are gold and teal
//   hoo         H-alpha to red, [O III] to green and blue; the two filter look
const float kLineColor[3][3][3] = {
  {{0.10f, 0.85f, 0.75f}, {1.00f, 0.16f, 0.26f}, {0.85f, 0.06f, 0.08f}},
  {{0.10f, 0.50f, 1.00f}, {0.55f, 0.90f, 0.30f}, {1.00f, 0.25f, 0.10f}},
  {{0.10f, 0.75f, 0.95f}, {1.00f, 0.18f, 0.12f}, {0.70f, 0.10f, 0.05f}},
};

#define REAL(n, h, m, lo, hi) Dial{n, h, &Settings::m, nullptr, nullptr, lo, hi}
#define INT(n, h, m, lo, hi) Dial{n, h, nullptr, &Settings::m, nullptr, lo, hi}
// Stepped by factors (see Dial::geometric); GEO0 also allows 0, as off.
#define GEO(n, h, m, lo, hi) Dial{n, h, &Settings::m, nullptr, nullptr, lo, hi, nullptr, 0, true, lo}
#define GEO0(n, h, m, floor, hi) \
  Dial{n, h, &Settings::m, nullptr, nullptr, 0.0f, hi, nullptr, 0, true, floor}
// Owned by a macro (see Dial::owner), stepped by amounts.
#define OWNED(n, h, m, lo, hi, owner) \
  Dial{n, h, &Settings::m, nullptr, nullptr, lo, hi, nullptr, 0, false, 0.0f, owner}
#define CHOICE(n, h, m, names) \
  Dial{n, h, nullptr, &Settings::m, nullptr, 0.0f, 0.0f, names, static_cast<int>(std::size(names))}

const Dial kDials[] = {
  Dial{"seed", "which sky; the same seed is always the same sky", nullptr, nullptr, &Settings::seed},
  REAL("viewer-offset", "viewer's distance from the bubble's centre in radii; under 1 is inside",
       viewerOffset, 0.0f, 4.0f),
  REAL("cluster-offset", "the lighting cluster's distance from the centre", clusterOffset, 0.0f,
       0.95f),
  GEO("luminosity", "the brightest cluster's output", luminosity, 0.01f, 100.0f),
  INT("clusters", "how many clusters light the main bubble", clusters, 1.0f,
      static_cast<float>(kMaxClusters)),
  GEO("thickness", "the shell's half thickness, in bubble radii", thickness, 0.005f, 0.5f),
  REAL("fold", "how far the shell is buckled, in bubble radii", fold, 0.0f, 1.0f),
  GEO("fold-scale", "how many folds across a radius", foldScale, 0.2f, 12.0f),
  REAL("keep", "how much of the shell survives its holes, 0..1", keep, 0.0f, 1.0f),
  GEO("hole-scale", "size of the holes and of the thickness variation", holeScale, 0.2f, 20.0f),
  GEO("outer-sharpness", "how much harder the outer edge is than the inner", outerSharpness,
       1.0f, 60.0f),

  GEO("detail-scale", "coarsest detail octave, cycles per bubble radius", detailScale, 0.5f,
       200.0f),
  REAL("detail-gain", "amplitude kept per octave; higher is rougher", detailGain, 0.2f, 0.9f),
  REAL("erosion", "how far the detail eats into the shell", erosion, 0.0f, 1.0f),
  REAL("filament", "0 billowed detail, 1 ridged filaments", filament, 0.0f, 1.0f),
  REAL("contrast",
       "spread of the gas's column density: 0 even fog, higher clear gaps and solid clouds",
       contrast, 0.0f, 3.0f),
  INT("pillars", "pillars on the main bubble's wall, pointing at its clusters", pillars, 0.0f,
      static_cast<float>(kMaxPillars)),
  GEO("pillar-length", "in bubble radii", pillarLength, 0.02f, 0.8f),
  GEO("pillar-width", "base radius, in bubble radii", pillarWidth, 0.005f, 0.2f),
  GEO("pillar-density", "relative to the shell's", pillarDensity, 0.1f, 20.0f),
  INT("clouds", "dark clouds adrift in the cavity, in front of the wall", clouds, 0.0f, 12.0f),
  GEO("cloud-length", "in bubble radii", cloudLength, 0.01f, 0.8f),
  GEO("cloud-width", "in bubble radii", cloudWidth, 0.003f, 0.2f),
  GEO("cloud-distance", "farthest from the viewer, in bubble radii", cloudDistance, 0.05f, 1.5f),
  GEO("cloud-density", "relative to the shell's", cloudDensity, 0.1f, 20.0f),
  REAL("hardness", "0 soft eroded edges, 1 crisp ones", hardness, 0.0f, 1.0f),
  CHOICE("dust-style",
         "flat dark lanes, soft veins, or physical: in the densest gas, shadowing and lit",
         dustStyle, kDustStyleNames),
  REAL("dust", "how much of the shell is dark molecular cloud", dust, 0.0f, 1.0f),
  GEO("dust-scale", "size of the dark clouds, cycles per bubble radius", dustScale, 0.2f, 40.0f),
  GEO0("cavity-density", "the ionised gas filling the cavity, whose glow is the heart",
       cavityDensity, 0.001f, 0.5f),
  GEO("cavity-spread",
       "how far each seed's cavity strays from cavity-density, a factor either way; 1 none",
       cavitySpread, 1.0f, 4.0f),
  CHOICE("form", "the main bubble: a thin shell, or a thick billowing mass round its cavity", form,
         kFormNames),
  CHOICE("distant-form", "the distant nebulae: shells, or masses seen from outside", distantForm,
         kFormNames),
  GEO("distant-density", "a distant mass's density, relative to the main mass's", distantDensity,
      0.01f, 4.0f),
  GEO0("distant-cavity", "a distant mass's cavity glow, seen through its walls; 0 none",
       distantCavity, 0.0005f, 0.5f),
  REAL("distant-blister", "how far a distant mass is blown open, 0..1", distantBlister, 0.0f, 1.0f),
  GEO0("distant-dust", "the galaxy's dust in front of the distant nebulae, dimming and reddening "
       "them; 0 none", distantDust, 0.1f, 20.0f),
  REAL("mass-inner", "the mass's inner surface, in bubble radii", massInner, 0.1f, 0.95f),
  REAL("mass-lobes", "how far the mass's lobes reach in over the cavity", massLobes, 0.0f, 0.8f),
  GEO("mass-scale", "the mass's billows, cycles per bubble radius", massScale, 0.5f, 20.0f),
  GEO("mass-density", "the mass's density, relative to the shell's", massDensity, 0.01f, 20.0f),
  INT("mass-clusters", "clusters lighting a mass: one leaves most of it facing away, in shadow",
      massClusters, 1.0f, static_cast<float>(kMaxClusters)),
  GEO0("mass-cavity", "a mass's cavity glow: gas in front of all of it, lifting its darks; 0 none",
       massCavity, 0.0005f, 0.5f),
  REAL("mass-fine", "fine lumps carved into a mass's surface; 0 none", massFine, 0.0f, 3.0f),
  OWNED("mass-billow", "how deep a mass's lumps are; 0 a smooth surface", massBillow, 0.0f, 3.0f,
        "billowing"),
  REAL("mass-edge", "how hard a mass's surface is: 0 soft fades, 1 crisp edges", massEdge, 0.0f,
       1.0f),
  REAL("mass-edge-patch",
       "where a mass's surface is hard: 0 all over, 1 only in patches, soft between",
       massEdgePatch, 0.0f, 1.0f),
  REAL("graze", "a mass's fine lumps shadowing each other toward the light; 0 none", graze, 0.0f,
       5.0f),
  REAL("mass-blister", "how far a mass's far side is blown out, 0..1", massBlister, 0.0f, 1.0f),
  GEO0("mass-cluster-size",
       "a mass's clusters' radius, bubble radii: softens their shadows; 0 a point",
       massClusterSize, 0.005f, 0.5f),
  REAL("mass-dust", "how much of a mass is dust; blind, it lost to none three times", massDust,
       0.0f, 1.0f),
  REAL("mass-warp", "how far the fold's swirl bends the mass, 0..1", massWarp, 0.0f, 1.0f),
  GEO0("cluster-size", "a cluster's radius, bubble radii: softens its shadows; 0 a point",
       clusterSize, 0.005f, 0.5f),
  GEO("fill-shadow", "how hard the fill light's small shadows are", fillShadow, 0.5f, 100.0f),
  GEO0("fill", "a mass's dim light raking across every face, shadowed only near at hand; 0 none",
       fill, 0.001f, 2.0f),
  REAL("blister", "how far the gas is blown out on the side away from the clusters, 0..1", blister,
       0.0f, 1.0f),
  INT("distant-count", "more distant nebulae beyond the main one", distantCount, 0.0f,
      static_cast<float>(kMaxBubbles - 1)),
  GEO("distant-min-deg", "smallest apparent radius of a distant one", distantMinDegrees, 0.2f,
       60.0f),
  GEO("distant-max-deg", "largest apparent radius of a distant one", distantMaxDegrees, 0.2f,
       80.0f),

  GEO("density", "multiplies all the gas", density, 0.01f, 50.0f),
  GEO("sigma", "extinction per bubble radius at density 1", sigma, 0.1f, 500.0f),
  GEO("oxygen-threshold", "flux per density above which [O III] takes over; lower is more teal",
       oxygenThreshold, 0.1f, 10000.0f),
  REAL("line-o", "[O III] strength", lineO, 0.0f, 4.0f),
  REAL("line-h", "H-alpha strength", lineH, 0.0f, 4.0f),
  REAL("line-s", "[S II] strength", lineS, 0.0f, 4.0f),
  CHOICE("line-colors", "how the three lines map to colour", lineColors, kLineColorNames),
  CHOICE("grade", "the sky's hue family: auto, by seed, or physical, the lines' own", grade,
         kGradeNames),
  REAL("grade-strength", "how far the colour goes toward the grade, 0..1", gradeStrength, 0.0f,
       1.0f),
  OWNED("grade-chroma", "a factor on the grade's palettes' chroma: 0 grey", gradeChroma, 0.0f,
        2.0f, "vivid"),
  GEO0("haze", "faint glow over all the sky: its darkest parts dim, not black", haze, 0.0005f, 0.2f),
  GEO0("shoulder", "ceiling the brightest gas eases toward, not clipped; 0 is off", shoulder, 0.1f,
       4.0f),
  REAL("grade-dust", "how far the dust takes its own colour, 0..1", gradeDust, 0.0f, 1.0f),
  CHOICE("hue-type",
         "one palette, a second fanned across the sky, or two regions of their own; auto by seed",
         hueType, kHueTypeNames),
  CHOICE("palette-family", "the auto grade's family of colour; auto by seed", paletteFamily,
         kFamilyNames),
  REAL("grade-galaxy", "how far the galaxy's band is graded, 0..1; 0 its own colour", gradeGalaxy,
       0.0f, 1.0f),
  REAL("grade-stars", "how far the stars' colours go toward the grade's, 0..1", gradeStars, 0.0f,
       1.0f),
  GEO0("reflection", "dust scattering the stars' own light", reflection, 0.01f, 10.0f),
  REAL("anisotropy", "Henyey-Greenstein g of the dust; positive is forward", anisotropy, -0.9f,
       0.9f),
  GEO("ion-opacity", "how much more opaque the gas is to ionising light; sharpens the fronts",
       ionOpacity, 1.0f, 200.0f),
  GEO0("dust-opacity", "extra extinction of the dark clouds", dustOpacity, 0.1f, 100.0f),
  REAL("rim-shadow", "fine self shadowing, for bright rims; 0 is off", rimShadow, 0.0f, 6.0f),

  GEO0("star-brightness", "all the stars together", starBrightness, 0.001f, 1000.0f),
  Dial{"star-count", "field stars, drawn from the galaxy about the observer", nullptr,
       &Settings::starCount, nullptr, 100.0f, 400000.0f, nullptr, 0, true, 100.0f},
  GEO("star-reach", "kpc; nearer stars are points, further ones glow", starReach, 0.1f, 10.0f),
  GEO("nebula-scale", "kpc per sky unit: the main bubble's radius", nebulaScale, 0.005f, 1.0f),
  GEO0("cluster-stars", "the lighting clusters' own stars", clusterStars, 0.01f, 100.0f),
  INT("young", "fainter young stars about each cluster", young, 0.0f, 500.0f),
  REAL("star-halo", "fraction of a star's light in its halo", starHalo, 0.0f, 1.0f),
  GEO("star-halo-deg", "the halo's angular radius", starHaloDegrees, 0.02f, 10.0f),
  REAL("spike", "fraction of the brightest stars' light in diffraction spikes", spike, 0.0f, 0.5f),
  GEO("spike-flux", "how bright a star must be to get spikes", spikeFlux, 1.0f, 100000.0f),
  CHOICE("nebula", "the nebula itself; off shows the galaxy and stars alone", nebula, kOnOffNames),
  CHOICE("galaxy-style", "the galaxy's structure", galaxyStyle, kGalaxyStyleNames),
  REAL("galaxy-radius",
       "our distance from the galaxy's centre, in disc scale lengths; the Sun is 3",
       galaxyRadius, 0.0f, 8.0f),
  REAL("galaxy-height", "kpc above the galaxy's midplane", galaxyHeight, -5.0f, 5.0f),
  GEO0("galaxy-glow", "the unresolved light of the galaxy's distant stars", galaxyGlow, 0.0001f, 10.0f),
  GEO0("galaxy-dust", "the galaxy's dust, which makes the rift", galaxyDust, 0.01f, 20.0f),
  REAL("galaxy-warp", "how much the outer disc is warped", galaxyWarp, 0.0f, 5.0f),
  REAL("galaxy-waves", "the bending waves rippling the outer disc", galaxyWaves, 0.0f, 5.0f),
  INT("external-galaxies", "other galaxies, far beyond this one", externalGalaxies, 0.0f,
      static_cast<float>(kMaxExternalGalaxies)),
  INT("galaxy-res", "texels per face of the galaxy's glow", galaxyRes, 64.0f, 2048.0f),

  GEO("exposure", "scales the nebula before the tonemapper", exposure, 0.0001f, 100.0f),
  GEO0("denoise", "tolerance of the filter that takes out the march's grain; 0 is off", denoise, 0.05f, 4.0f),
  INT("supersample", "rays per texel along each side, box averaged down", supersample, 1.0f, 2.0f),
  GEO("step-frac", "march step as a fraction of the shell's thickness", stepFrac, 0.02f, 1.0f),
  INT("max-steps", "march steps allowed per bubble", maxSteps, 50.0f, 4000.0f),
  INT("light-res", "voxels per side of each bubble's light volume", lightRes, 16.0f, 256.0f),
  INT("light-steps", "march steps from each voxel to its cluster", lightSteps, 8.0f, 256.0f),
};

#undef REAL
#undef INT
#undef GEO
#undef GEO0
#undef OWNED
#undef CHOICE

// A hash of the seed, -1..1: how much the cavity glows, to be spread either way
// of its dial. The hot ionised gas filling the cavity is the smooth bright
// heart of an HII region, the calm mass its structure is seen against. How
// much of it is a matter of taste that differs from sky to sky: judged by eye
// on three seeds, one was best as faint as 0.002 and two at 0.008.
float cavityBySeed(uint32_t seed) {
  uint32_t h = seed * 2654435761u + 0x7f4a7c15u;
  h ^= h >> 15;
  h *= 0x2c1b3c6du;
  h ^= h >> 12;
  h *= 0x297a2d39u;
  h ^= h >> 15;
  return static_cast<float>(h >> 8) / 8388608.0f - 1.0f;
}

}  // namespace

const Dial* dials(int& count) {
  count = static_cast<int>(std::size(kDials));
  return kDials;
}

bool setDial(Settings& s, const std::string& name, const std::string& value, std::string& error) {
  for (const Dial& d : kDials) {
    if (name != d.name) {
      continue;
    }
    if (d.owner) {
      error = name + " is set by the " + d.owner + " macro, not on its own";
      return false;
    }
    if (d.choices) {
      for (int i = 0; i < d.choiceCount; i++) {
        if (value == d.choices[i]) {
          s.*d.integer = i;
          return true;
        }
      }
      error = name + ": '" + value + "' is not one of";
      for (int i = 0; i < d.choiceCount; i++) {
        error += std::string(" ") + d.choices[i];
      }
      return false;
    }
    const char* text = value.c_str();
    char* end = nullptr;
    errno = 0;
    if (d.seed) {
      unsigned long long v = std::strtoull(text, &end, 10);
      if (value.empty() || *end || errno || v > 0xffffffffull) {
        error = name + ": '" + value + "' is not a whole number from 0 to 4294967295";
        return false;
      }
      s.*d.seed = static_cast<uint32_t>(v);
      return true;
    }
    double v = d.integer ? static_cast<double>(std::strtol(text, &end, 10)) : std::strtod(text, &end);
    if (value.empty() || *end || errno || !std::isfinite(v)) {
      error = name + ": '" + value + "' is not " + (d.integer ? "a whole number" : "a number");
      return false;
    }
    // Compared as the float it will be stored as: "0.9" read as a double is a
    // little more than the dial's own limit 0.9f, and was refused.
    float stored = static_cast<float>(v);
    if (stored < d.lo || stored > d.hi) {
      char range[64];
      std::snprintf(range, sizeof(range), "%g .. %g", static_cast<double>(d.lo),
                    static_cast<double>(d.hi));
      error = name + ": " + value + " is outside " + range;
      return false;
    }
    if (d.integer) {
      s.*d.integer = static_cast<int>(v);
    } else {
      s.*d.real = static_cast<float>(v);
    }
    return true;
  }
  error = "no dial called '" + name + "'";
  return false;
}

std::string dialValue(const Settings& s, const Dial& d) {
  if (d.seed) {
    return std::to_string(s.*d.seed);
  }
  if (d.choices) {
    int i = s.*d.integer;
    return i >= 0 && i < d.choiceCount ? d.choices[i] : "?";
  }
  if (d.integer) {
    return std::to_string(s.*d.integer);
  }
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(s.*d.real));
  return buf;
}

Sky buildSky(const Settings& s) {
  Sky sky{};
  sky.distantDust = s.distantDust;
  SceneParams& p = sky.scene;
  p.seed = s.seed;
  p.viewerOffset = s.viewerOffset;
  p.clusterOffset = s.clusterOffset;
  p.luminosity = s.luminosity;
  p.clusters = s.clusters;
  // A mass is lit its own way: by one cluster, so most of it turns from the
  // light and only part of it catches it. Two or three inside light its whole
  // inner face like a lamp in a globe -- measured, three times the reference
  // skies' share of middle tones -- and one without a glowing cavity, which
  // lies in front of all of it, scored best of three lightings blind.
  if (s.form == 1) {
    p.clusters = s.massClusters;
  }
  p.thickness = s.thickness;
  p.fold = s.fold;
  p.keep = s.keep;
  p.pillars = s.pillars;
  p.pillarLength = s.pillarLength;
  p.pillarWidth = s.pillarWidth;
  p.clouds = s.clouds;
  p.cloudLength = s.cloudLength;
  p.cloudWidth = s.cloudWidth;
  p.cloudDistance = s.cloudDistance;
  p.distantCount = s.distantCount;
  p.distantMinDegrees = s.distantMinDegrees;
  p.distantMaxDegrees = std::fmax(s.distantMaxDegrees, s.distantMinDegrees);

  Look& l = sky.look;
  l.foldScale = s.foldScale;
  l.outerSharpness = s.outerSharpness;
  l.holeScale = s.holeScale;
  l.cavityDensity = s.form == 1 ? s.massCavity
                                : s.cavityDensity * powf(s.cavitySpread, cavityBySeed(s.seed));
  // A mass has its own settings where they were tuned apart from the shell's,
  // blind: less blister, soft shadows, and no dust, which lost to none three
  // times.
  l.blister = s.form == 1 ? s.massBlister : s.blister;
  l.form = s.form;
  l.distantForm = s.distantForm;
  l.distantDensity = s.distantDensity;
  l.distantCavity = s.distantCavity;
  l.distantBlister = s.distantBlister;
  l.massInner = s.massInner;
  l.massLobes = s.massLobes;
  l.massScale = s.massScale;
  l.massDensity = s.massDensity;
  l.massWarp = s.massWarp;
  l.massFine = s.massFine;
  l.massBillow = s.massBillow;
  l.massEdge = s.massEdge;
  l.massEdgePatch = s.massEdgePatch;
  l.graze = s.graze;
  // The raking fill is the mass's: a shell's thin sheet has no unlit bulk.
  l.fill = s.form == 1 ? s.fill : 0.0f;
  l.fillShadow = s.fillShadow;
  l.clusterSize = s.form == 1 ? s.massClusterSize : s.clusterSize;
  l.detailScale = s.detailScale;
  l.detailGain = s.detailGain;
  l.erosion = s.erosion;
  l.filament = s.filament;
  l.hardness = s.hardness;
  l.contrast = s.contrast;
  l.pillarDensity = s.pillarDensity;
  l.cloudDensity = s.cloudDensity;
  l.dustAmount = s.form == 1 ? s.massDust : s.dust;
  l.dustScale = s.dustScale;
  l.dustStyle = s.dustStyle;
  l.density = s.density;
  l.sigma = s.sigma;
  std::memcpy(l.lineColor, kLineColor[s.lineColors], sizeof(l.lineColor));
  l.lineStrength[0] = s.lineO;
  l.lineStrength[1] = s.lineH;
  l.lineStrength[2] = s.lineS;
  l.oxygenThreshold = s.oxygenThreshold;
  // Interstellar dust scatters blue better than red, which is why reflection
  // nebulae are blue; and absorbs it better, which is why stars behind dust
  // are reddened.
  l.dustAlbedo[0] = 0.45f;
  l.dustAlbedo[1] = 0.6f;
  l.dustAlbedo[2] = 0.9f;
  l.reddening[0] = 0.8f;
  l.reddening[1] = 1.0f;
  l.reddening[2] = 1.3f;
  l.anisotropy = s.anisotropy;
  l.reflection = s.reflection;
  l.rimShadow = s.rimShadow;
  l.ionOpacity = s.ionOpacity;
  l.dustOpacity = s.dustOpacity;
  l.starBrightness = s.starBrightness;
  l.starHalo = s.starHalo;
  l.starHaloDegrees = s.starHaloDegrees;
  l.starSpike = s.spike;
  l.starSpikeFlux = s.spikeFlux;
  l.galaxyGlow = s.galaxyGlow;
  l.stepFrac = s.stepFrac;
  l.maxSteps = s.maxSteps;
  l.lightRes = s.lightRes;
  l.lightSteps = s.lightSteps;
  l.exposure = s.exposure;
  PaletteChoice choice;
  choice.grade = s.grade == 1;
  choice.family = s.paletteFamily - 1;
  choice.strength = s.gradeStrength;
  choice.chroma = s.gradeChroma;
  choice.dust = s.gradeDust;
  choice.hueType = s.hueType;
  buildGrade(s.seed, choice, l);
  l.starGrade = s.grade == 0 ? 0.0f : s.gradeStars;
  l.gradeGalaxy = s.gradeGalaxy;
  // The haze takes the grade's darkest colour.
  for (int i = 0; i < 3; i++) {
    l.haze[i] = s.haze * l.ramp[0][i];
  }
  l.shoulder = s.shoulder;
  l.denoise = s.denoise;
  l.supersample = s.supersample;

  GalaxyParams& g = sky.galaxy;
  g.style = s.galaxyStyle;
  g.observerRadius = s.galaxyRadius;
  g.observerHeight = s.galaxyHeight;
  g.dust = s.galaxyDust;
  g.warp = s.galaxyWarp;
  g.waves = s.galaxyWaves;
  g.externalGalaxies = s.externalGalaxies;
  sky.galaxyRes = s.galaxyRes;

  StarParams& st = sky.stars;
  st.count = s.starCount;
  st.reach = s.starReach;
  st.kpcPerSkyUnit = s.nebulaScale;
  std::memcpy(st.reddening, l.reddening, sizeof(st.reddening));
  st.youngPerCluster = s.young;
  st.clusterBrightness = s.clusterStars;
  sky.nebula = s.nebula != 0;
  return sky;
}

}  // namespace starcanopy
