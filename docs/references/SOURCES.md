# Sources

Where each model and each fitted constant in StarCanopy came from
(`PRINCIPLES §10`). A number without a source here is a number nobody can
defend or safely change.

**Reference skies are used as numbers only.** Brightness distributions, and
hue and chroma by lightness, are measured from them; the images themselves are
never committed, at any point in the history, including in docs, fixtures and
test data (`fence #4`). Nothing in this file is a picture of one, and nothing
should be.

**Whether a number tracks the look is recorded with it.** A measurement that
has not been checked against blind scores is a hypothesis (`PRINCIPLES §2`),
and says so in the *Checked* column.

Everything below was measured or fitted in the Space Nerds In Space labs
(`labs/features/nebula_sky`, branch `nebula-sky`); the commit named is where the
numbers and their method are written up in full. Entries are rewritten to name
StarCanopy's own code as it arrives in the lift.

## The reference skies

The aesthetic standard is the skyboxes of a published space game: the level of
look to reach, not a sky to reproduce (`PRINCIPLES §2`). That game is named once,
in `README.md`, and nowhere else (`AGENTS.md`). Measured:

- **three full skyboxes**, as 8192 x 4096 equirectangular images;
- **twelve nebula images** from the same game;
- **23 images** in all for the palette space (below), which widened the first
  fifteen.

**How they were obtained.** All were publicly available through web image
search, mostly as desktop backgrounds people had made from them. They were
measured, and are not redistributed: not here, and not anywhere by this project.

Measurement is in **OKLab** (Björn Ottosson, *A perceptual color space for
image processing*, 2020), where lightness, chroma and hue are as the eye sees
them, usually in ten bands of lightness.

*To record:* where the measurement scripts live; they are not in the labs'
history.

## Fitted to the references

| What | Numbers | From | Checked | Labs |
|---|---|---|---|---|
| **Palette shape.** Every reference is one curve through colour: hue nearly constant within a band of lightness (spread under 0.1 of the circle in 13 of 15); chroma an arch peaking in the midtones (peak 0.02-0.1 in 13 of 15); hue drifting toward yellow, about 95 degrees, in the highlights, except blues, which keep theirs. | the model of a palette as a path (`DESIGN.md §4.1`) | 15 references, OKLab, ten lightness bands | Not blind-scored as such; the palette space built on it was (next row) | `3749dd2e`, 2026-09-23 |
| **Palette space.** Four families by dark-end hue -- warm 17-63 degrees, green 112-152, teal 182-198, blue 228-258 -- and no reference in the gaps. Warm, green and teal turn from lightness 0.25-0.45 toward a yellow-white of 92-110 degrees; blue eases 8-32 degrees toward cyan. Peak chroma 0.08-0.19 warm, 0.03-0.10 the cool families. Family weights are how often each leads: warm 0.57, blue 0.21, green 0.13, teal 0.09. Dust hue moves toward brown by family: warm 0.3-0.6, green 0.6-1.0, teal 0.5-0.9, blue 0-0.2. | `palette_family[]` in the labs | 23 references | By eye: a blind sheet of references and drawn palettes mixed showed none out of place. Not a scored A/B. | `71272537`, 2026-09-25 |
| **Tone.** Whole sky, area weighted, CIE L*: 10th percentile 1-4, median 5-16, 90th 17-52; 37-44% of the light in the brightest 10% of the sky. | haze and shoulder were tuned toward these (`DESIGN.md §4.2`) | the three skyboxes | **Does not track blind scores.** A candidate that matched these bands far better than the default lost 10-0, because it hid the open sky. Explanatory only. | `3749dd2e`; labs `NEXT.md` |
| **Mass lighting.** Two or three clusters inside a mass light its whole inner face; measured, that gave three times the references' share of middle tones. | one cluster lights a mass (`DESIGN.md §3.1`) | the three skyboxes | Yes: one cluster without a glowing cavity scored best of three lightings | labs `lab_nebula_sky_feature.c` |
| **Star colour.** Measured as each star's excess over the sky around it, the references' stars gather at the palette's hue and are at least as colourful as its gas. | stars tinted with the palette's midtones, at luminance 1 | the three skyboxes | Not separately | labs `nebula_sky_bake.c`, `star_tint()` |

| **Edge width.** Median strong-boundary width (step over steepest slope) at 45 degree field of view: the references' 640-per-face copies 2.03 degrees; ours cut to 640, 1.76; ours at 1024, 1.64. | the *Hard edges* work (`ROADMAP.md`) | 45 degree views cut from 8 reference skies and 6 of ours | **Cannot settle the question**: 640-per-face copies do not show those skies as a game does. Judge by eye. | labs `NEXT.md` |

## Chosen by eye, not fitted

Constants set by looking, recorded so nobody mistakes them for measurements.

| What | Value | How |
|---|---|---|
| Cavity glow | 0.002-0.008 by seed | judged on three seeds: one best at 0.002, two at 0.008 |
| Soft shadows | five points, power mean of six taps, exponent 0.3 | keeps the sky's mean brightness within a few percent of point sources', measured on two seeds |
| Raking fill, grazing shadows, no dust | on, on, off | blind A/Bs (`DESIGN.md §3.1`, `PRINCIPLES §3`) |

## Models

| Model | Source | Status |
|---|---|---|
| Deterministic generator | `mtwist.c` from Space Nerds In Space (Stephen M. Cameron), GPL-2.0-or-later | Kept so seeds match the labs (`THIRDPARTY.md`) |
| OKLab | Björn Ottosson, 2020 | Used for measuring and for palettes |
| Emission by ionisation parameter: [O III], H-alpha, [S II] | Standard H II region physics; no specific reference recorded in the labs | *To record* in the lift |
| Galaxy: exponential flaring disc, bar, bulge, arms, warp, bending waves, dust | Described in the labs' `nebula_sky_galaxy.h`; no literature cited for its structure or constants | *To record* in the lift |

## For the lift: what fence #4 does not let through

The labs had two things aimed at the references directly, useful there for
comparison. **Neither is lifted:**

- **A comparison star field**, drawn to match the references' own star fields.
  It was written from measurements alone, but as an option here it would be a
  preset imitating a named product, and not worth the ambiguity. Its
  measurements are not carried over either.
- **Named palettes**, each fitted to one reference sky. Only the palette space
  above is lifted, which draws from the spread of all of them. No palette,
  parameter, comment or commit message on this side names a particular reference
  sky, or says which one anything was fitted to.
