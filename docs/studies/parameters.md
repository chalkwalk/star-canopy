# The parameters: triage and sensitivity

*Study of look 1, 2026-09-26/27. Tools: `tools/study` -- `study` bakes and
measures, `analyse.py` and `triage.py` make the tables below. Data:
`docs/studies/data/`.*

The model has a hundred raw dials. The macros are to be made of them
(`PRINCIPLES §5`, `DESIGN.md` §5), and before a macro can be designed it has to be
known which dials do anything, how much, in which direction, and which only do
something together. This is that study. It decides nothing about the look: it
measures how dials **change** the sky, never whether a change is **better**
(`PRINCIPLES §2`). Which way a macro should turn a sky is for blind scoring.

## Summary

- Of 99 dials (the seed aside), **56 are look dials**: they visibly move a sky of
  one style or the other. Thirteen of them are the mass's own.
- **25 retire with the shell** (`ROADMAP.md`, *The shell's retirement, and
  bubbles*): read only by the shell, the mass having twins or skipping them.
- **5 are off by a decision** (blind results, or a blind test pending), **3 are
  debug views**, **7 trade time for fidelity**, and **3 change nothing visible**
  at any size studied: the clusters' own stars, and the external galaxies. Those
  three want a look by eye before anything is removed.
- The dials that move the sky most, by far, say **where things are**: where the
  observer sits in the galaxy, where the light is, where the viewer stands in the
  nebula -- composition, not texture.
- **Openness**, the sky's clear share, has many levers pulling both ways; a macro
  for it is well supplied. **Detail** dials are many, each modest, and all
  **resolution-stable**: they move a sky at 512 a face as they do at 128.
- The light volume's resolution is **not converged**: 76 or 136 voxels instead of
  96 changes the sky as much as each other. A lighter light volume for previews
  would not be the same sky (`PRINCIPLES §9`) until that is fixed.
- The study found two bugs on the way, both fixed with tests (below).

## Method

**Skies.** Look 1 at 128 texels a face -- the size of the Explore grid, where the
macros will first be used -- with every other dial at its default, in each style
separately, since the style decides which dials are read at all. The dials that
matter for fine structure and points were baked again at 512 a face (below).

**Change.** Two skies are compared as displayed: each channel through the display
curve the look was judged through, and

- **reach**: the mean absolute difference over the sphere in 8-bit display levels,
  each texel weighted by its solid angle (so the cube's corners count no more than
  its middles). Under 0.05 levels is none: exactly 0 where a dial is never read,
  floating-point noise otherwise;
- **sky changed**: the share of the sphere where some channel changed by more
  than two levels -- visibly. The mean hardly sees a change to points (the stars)
  or to structure finer than a texel; this sees how much of the sky changed at all.

Both measure how much, over the whole sky; neither says anything about edges or
detail at a game's field of view (`PRINCIPLES §1`), nor about better.

**Descriptors,** to say how a sky changed, each over the sphere, as displayed,
solid-angle weighted:

| descriptor | what it is |
|---|---|
| bright | mean displayed lightness (OKLab L) |
| contr | its standard deviation |
| clear | share of the sky the gas hides almost nothing of (transmittance over 0.9) |
| opaque | share it hides almost everything of (under 0.1) |
| chroma | mean OKLab chroma |
| detail | mean difference of a texel's lightness from its four neighbours' |
| top10 | share of the linear light in the brightest tenth of the sky |

None has been checked against blind scores: they explain, they do not judge.

**One at a time** (`study oat`, 1664 bakes a style): from the defaults, each dial
swept on seeds 1, 3, 5 and 7 -- a dial stepped by amounts at five points across its
whole range; one stepped by factors (`Dial::geometric`) at a quarter, a half, twice
and four times its default, and at 0 where 0 means off -- each bake compared with
the same seed's default sky. A dial's **direction** on a descriptor is the mean
effect of turning it up (values above the default against values below), counted
only where it exceeds a quarter of the spread of that descriptor between the four
seeds, so seed noise cannot pass for an effect; the count after it is how many
seeds agree.

**At 512 a face** (298 bakes a style): the 34 dials of fine structure, stars and
distant nebulae, swept the same way on seeds 1 and 7, to see whether 128 a face
hides what they do.

**Morris's elementary effects** (`study morris`, 720 bakes a style): see
*Interactions*.

**Two bugs**, found because the study's own numbers disagreed with themselves,
and fixed before these data were taken:

- `setDial` refused sixteen dials' own range ends -- "0.9" read as a double is a
  little more than the float limit 0.9f (`bc74130`).
- A baker used for a second sky drew a broken galaxy: GL clips drawing to the
  smallest attachment of a framebuffer, and the last march's targets, still
  attached, cut the galaxy pass to a quarter of each face (`afcbc20`). Single
  renders were never affected; the study, and the interface to come, were. Every
  number here is from the fixed baker, and each seed's default sky matches a fresh
  process's bake exactly.

## What moves the sky

From the defaults, in order of reach, the look dials of each style (full numbers
in the table at the end; directions in `analyse.py oat`):

**Where things are** leads both styles by a wide margin. `galaxy-radius` -- the
observer's distance from the galaxy's centre -- reaches 47 levels on the mass and
104 on the shell, the whole sky's light moving between a galaxy's bright centre
and its dark edge; `galaxy-height` and `galaxy-glow` follow it. Then the light and
the viewer: `cluster-offset` (31 on the mass: a cluster further out lights less of
it, the sky darker and less colourful), `luminosity`, `viewer-offset` (22: stand
further toward the edge and the sky opens, clear up, opaque down, on every seed),
and on the mass `mass-clusters` and the cavity's glow, `mass-cavity`. And
`exposure`, which scales everything. These are composition: a macro set will want
them as its coarsest controls, or leave them to the seed (`PRINCIPLES §6`).

**Openness** -- clear up, opaque down -- is moved one way by `viewer-offset`,
`mass-inner`, `mass-blister`, `contrast` and `erosion`, and the other by
`mass-scale`, `keep`, `mass-lobes`, `mass-density`, `density`, `sigma`,
`mass-fine` and `detail-scale`, each consistently on all four seeds. An *open /
enveloping* macro has more than enough to be made of.

**Brightness** without composition: the line strengths `line-s` (24 levels on the
mass) and `line-h` (13) -- the grade takes the lines' colour, so what they set is
how bright the lower-ionisation gas glows -- `ion-opacity`, `fill` and `haze`. The
haze brightens the whole sky and flattens it (contrast down on every seed); the
fill brightens the mass's faces turned from the light and adds detail. `line-o`
does almost nothing on the mass (0.4) and a lot on the shell (12): the mass's gas
is too dense and too far from its cluster to be doubly ionised.

**Texture**: `mass-fine`, `fold-scale`, `detail-scale`, `filament`, `hardness`,
`detail-gain`, `erosion`, `graze`, and the pending `mass-edge`. Each moves 2-11
levels over two fifths to two thirds of the sky -- small everywhere rather than
large anywhere, as fine structure should.

**Colour**: `palette-family`, `grade-strength`, `hue-type`, `grade-galaxy` and, a
little, `grade-stars`. The palette family's reach (9-12) is the size of a change of
hue family, not of any one family's range.

**The stars**: `star-count`, `star-brightness` and `star-reach` change a seventh
to a half of the sky at 128 a face -- every star texel -- and about half as much at
512, where each star is a smaller share of the sky. `star-halo` changes little at
either, but more at 512, a halo being sized in angle. Whole-sky numbers undersell points; stars
want looking at.

## Resolution

The fine-structure dials move a sky at 512 a face as they do at 128: `mass-fine`,
`mass-edge`, `fold-scale`, `detail-scale`, `graze`, `filament`, `erosion`,
`hardness` and `detail-gain` reach within about 10% of the same, over the same
share of the sky, at both sizes. So the octave limit does what `PRINCIPLES §9` asks: a
small sky is the same sky seen more coarsely, and a macro tuned in the Explore grid
does at export what it did there.

## Quality: what fidelity costs

At 128 a face, a bake that reuses its light volume takes 1.4 s for the mass and
0.7 s for the shell; the light volume at its default resolution adds 3.7 s and
0.2 s. The quality dials, each against the default sky:

| dial (default) | values: reach, sky changed, seconds a bake (mass) |
|---|---|
| `light-res` (96) | 16: 5.6, 53%, 1.5 -- 76: 0.43, 8%, 3.5 -- 136: 0.38, 6%, 11 -- 196: 0.56, 12%, 27 -- 256: 0.64, 14%, 53 |
| `light-steps` (48) | 8: 4.1, 41%, 2.1 -- 70: 0.05, 0%, 6.7 -- 132 to 256: under 0.1, 0% |
| `max-steps` (600) | 50: 2.8, 20% -- 1038 to 4000: 0, 0% |
| `step-frac` (0.15) | 0.0375: 0.18, 1%, 3.2 -- 0.075: 0.17, 1%, 2.0 -- 0.3: 0.52, 8% -- 0.6: 1.8, 29% |
| `supersample` (2) | 1: 0.54, 8%, 0.8 |
| `denoise` (0.35) | 0: 0.87, 18% -- 0.7: 0.61, 11% -- 1.4: 0.93, 15% |
| `galaxy-res` (512) | 64: 0.05, 1% -- 560 to 2048: 0, 0% |

- **`light-steps` has converged** at 48 and **`max-steps` never binds** at 600:
  both could be lower for nothing, or left alone for safety.
- **`galaxy-res` is inert at this face size** down to 64: the Explore grid could
  bake a tiny galaxy. Whether 512 is enough at 2048 a face is not measured here.
- **`light-res` is not converged.** 76, 136, 196 and 256 each move the sky 0.4-0.6
  levels from 96 and visibly change 6-14% of it, with no trend toward agreement:
  each voxel resolution puts the shadows a little differently, and none is the
  limit the others approach. So `DESIGN.md` §9's hope -- a lighter light volume
  for small previews that gives the same sky -- cannot be met by resolution alone:
  the light volume would first have to be made resolution-stable (filtered to its
  voxel, as the field's octaves are to the texel). That is work for *Preview
  ladder*, and a look change for a new look version.
- Stepping finer than the default (`step-frac` 0.0375) changes 1% of the sky: the
  march is close to converged where it matters.

## Interactions

**Morris's elementary effects** (`study morris`, 720 bakes a style): eight
trajectories a style, each from a random point in a usable space of the look dials
-- a quarter of a dial's range either side of its default, or a factor of two
either way, at four levels -- on a random seed, stepping every dial once by two
levels. A dial's **mu\*** is its mean change per step, anywhere in that space;
its **sigma**, the spread of those changes. The quality and debug dials stay at
their defaults.

The space had to be narrowed to mean anything. A first run over the one-at-a-time
intervals, every dial at once, made nearly every point an empty sky -- the median
point was 100% clear in both styles -- and so described empty skies. Narrowed, its
skies are real ones: the shell's median point is 71% clear against its defaults'
72%; the mass's is more open than its defaults (57% against 25%) but spans them,
from 5% to 87%. Its mu\* are smaller than the reaches above, being steps of two
thirds of a narrow interval rather than sweeps of a wide one; they compare dials
with each other, not with the reaches.

**Nothing matters only together that was not known to.** The dials with no reach
from the defaults but an effect elsewhere are, on the mass, the dust's
(`dust-style`, `dust-opacity`, `dust-scale`), which wait on `mass-dust`;
`mass-edge-patch`, which waits on `mass-edge`; and `distant-min-deg`, whose
nebulae show only where the mass opens up. Every such gate is in the code, and in
the triage.

**How independent a dial is** is what matters for macros, whose contributions add
(`DESIGN.md` §5). A sigma as large as mu\* means a dial's effect depends on where
the others are; a small one, that it adds up. Among dials with mu\* of a level or
more:

| | mass | shell |
|---|---|---|
| most dependent (sigma / mu\*) | `fold-scale` 1.8, `hue-type` 1.7, `mass-blister` 1.6, `mass-lobes` 1.5, `ion-opacity` 1.4, `mass-cavity` 1.3, `cluster-offset` 1.3, `mass-fine` 1.2 | `dust-scale` 1.7, `hue-type` 1.6, `pillars` 1.5, `thickness` 1.5, `cloud-distance` 1.3, `ion-opacity` 1.2, `detail-scale` 1.2, `contrast` 1.2 |
| most additive | `haze` 0.3, `mass-scale` 0.4, `palette-family` 0.5, `mass-density` 0.6, `contrast` 0.6, `line-h` 0.6, `exposure` 0.7, `keep` 0.7 | `haze` 0.3, `sigma` 0.4, `exposure` 0.4, `keep` 0.5, `galaxy-radius` 0.6, `density` 0.6, `blister` 0.7, `palette-family` 0.7 |

So the easy material for additive macros is the haze, the exposure, the density
of the gas and the mass's scale, the holes, the palette. The shape of the mass's
inner surface -- its blister and lobes -- the cavity's glow, where the cluster
sits and the ionising opacity interact strongly: a macro built of them wants its
curves tuned by looking at many skies, not assumed to add. `viewer-offset` and
`galaxy-height` have the largest mu\* of all (34 and 23 on the mass) and
sigma to match: what they do depends on everything, as where one stands does.

## Triage

Every dial, its class, and the numbers it was decided from. The class is a
decision made from the numbers, written in `triage.py` with its reason; the script
flags any class the numbers contradict, so a rerun cannot silently disagree with
the triage. Reach is in display levels, from the defaults, and *sky changed* the
largest visibly changed share of any value swept, both at 128 a face (the 512 a
face follow-up is under *Resolution*, and the "no visible effect" class holds at
both); mu\* is from *Interactions*, where the dial took part.

- **look**: moves a sky visibly; the material macros are made of.
- **shell style (retiring)**: read only by the shell (or, for the distant
  nebulae, hidden by the mass); goes with the shell.
- **off by a decision**: off by default because of a blind result, or waiting
  for one.
- **no visible effect**: changes under half a percent of the sky at any value, in
  either style, at either size. To be looked at by eye before removal.
- **debug**: a view for judging the physics.
- **quality**: trades time for fidelity; not for macros.

| dial | default | class | mass: reach, sky changed | shell: reach, sky changed | mu* mass / shell | note |
|---|---|---|---|---|---|---|
| galaxy-radius | 3.5 | look | 47.46, 68% | 103.55, 99% | 16.28 / 20.66 |  |
| exposure | 0.18 | look | 38.64, 95% | 55.76, 100% | 18.01 / 16.63 |  |
| galaxy-glow | 1.5 | look | 15.00, 56% | 31.22, 95% | 6.52 / 9.64 |  |
| cluster-offset | 0.85 | look | 30.51, 83% | 18.88, 64% | 10.88 / 6.59 |  |
| luminosity | 1 | look | 23.64, 75% | 27.78, 71% | 5.33 / 7.30 |  |
| mass-cavity | 0 | look | 26.41, 78% | 0.00, 0% | 3.76 / 0.00 |  |
| viewer-offset | 0.55 | look | 22.31, 91% | 25.01, 77% | 33.83 / 16.31 |  |
| line-s | 0.5 | look | 23.58, 74% | 12.49, 48% | 4.89 / 2.46 |  |
| galaxy-height | 0.03 | look | 10.99, 49% | 22.89, 89% | 23.30 / 25.19 |  |
| mass-clusters | 1 | look | 22.01, 77% | 0.00, 0% | 9.03 / 0.00 |  |
| mass-scale | 1.5 | look | 17.11, 88% | 0.00, 0% | 9.11 / 0.00 |  |
| keep | 0.7 | look | 16.93, 80% | 9.11, 54% | 3.19 / 2.44 |  |
| fold | 0.18 | look | 13.45, 77% | 16.57, 69% | 2.78 / 5.05 |  |
| haze | 0.004 | look | 16.09, 100% | 13.77, 99% | 5.18 / 5.24 |  |
| mass-inner | 0.75 | look | 15.85, 76% | 0.00, 0% | 13.41 / 0.00 |  |
| mass-blister | 0.3 | look | 14.62, 73% | 0.00, 0% | 2.80 / 0.00 |  |
| line-h | 1 | look | 13.47, 66% | 12.18, 58% | 4.39 / 4.60 |  |
| hole-scale | 1.3 | look | 10.84, 74% | 12.07, 65% | 6.43 / 5.53 |  |
| palette-family | auto | look | 9.24, 72% | 11.70, 75% | 13.61 / 14.62 |  |
| line-o | 1 | look | 0.42, 3% | 11.58, 48% | 0.88 / 3.20 |  |
| mass-density | 6 | look | 11.48, 74% | 0.00, 0% | 5.04 / 0.00 |  |
| density | 1 | look | 11.43, 75% | 8.36, 57% | 4.82 / 3.30 |  |
| sigma | 8 | look | 11.39, 75% | 9.07, 57% | 4.46 / 4.29 |  |
| fold-scale | 1.6 | look | 7.16, 63% | 11.29, 61% | 5.25 / 6.83 |  |
| grade-strength | 1 | look | 11.19, 76% | 9.73, 78% | 0.99 / 1.20 |  |
| mass-warp | 0.25 | look | 11.00, 72% | 0.00, 0% | 2.21 / 0.00 |  |
| mass-lobes | 0.4 | look | 10.74, 67% | 0.00, 0% | 3.08 / 0.00 |  |
| contrast | 1.5 | look | 10.42, 64% | 8.04, 54% | 3.43 / 3.99 |  |
| ion-opacity | 8 | look | 9.94, 58% | 8.56, 55% | 6.77 / 4.98 |  |
| erosion | 0.65 | look | 3.96, 50% | 9.84, 73% | 0.95 / 0.60 |  |
| fill | 0.1 | look | 9.44, 54% | 0.00, 0% | 1.14 / 0.00 |  |
| mass-fine | 1 | look | 8.41, 68% | 0.00, 0% | 2.21 / 0.00 |  |
| detail-scale | 5 | look | 5.73, 62% | 8.04, 58% | 2.73 / 3.96 |  |
| fill-shadow | 15 | look | 7.18, 67% | 0.00, 0% | 0.77 / 0.00 |  |
| galaxy-waves | 1 | look | 2.94, 36% | 6.63, 73% | 0.51 / 0.80 |  |
| hue-type | auto | look | 5.21, 29% | 6.54, 34% | 3.76 / 2.54 |  |
| star-count | 30000 | look | 2.93, 28% | 6.29, 56% | 1.16 / 1.38 |  |
| galaxy-style | barred-spiral | look | 2.92, 32% | 6.25, 63% | 2.78 / 3.27 |  |
| graze | 3 | look | 5.92, 56% | 0.00, 0% | 1.41 / 0.00 |  |
| filament | 0.5 | look | 3.56, 50% | 4.99, 50% | 0.72 / 0.94 |  |
| star-brightness | 0.55 | look | 2.28, 22% | 4.94, 44% | 0.75 / 0.88 |  |
| hardness | 0.5 | look | 2.46, 41% | 4.45, 44% | 0.89 / 1.97 |  |
| grade-galaxy | 0.2 | look | 2.21, 37% | 4.37, 75% | 1.32 / 1.70 |  |
| mass-cluster-size | 0.15 | look | 4.20, 55% | 0.00, 0% | 3.43 / 0.00 |  |
| detail-gain | 0.55 | look | 2.23, 41% | 3.58, 41% | 0.57 / 1.30 |  |
| galaxy-dust | 1 | look | 1.90, 19% | 3.54, 34% | 0.25 / 0.28 |  |
| star-reach | 1.5 | look | 1.48, 14% | 3.20, 32% | 1.50 / 1.95 |  |
| oxygen-threshold | 20 | look | 0.12, 2% | 1.94, 27% | 0.92 / 1.46 |  |
| shoulder | 0.8 | look | 0.62, 4% | 1.86, 10% | 0.30 / 0.48 |  |
| reflection | 0.2 | look | 1.66, 31% | 0.76, 14% | 0.38 / 0.25 |  |
| anisotropy | 0.5 | look | 1.58, 36% | 0.48, 10% | 0.73 / 0.53 |  |
| galaxy-warp | 1 | look | 0.41, 5% | 1.06, 15% | 0.29 / 0.35 |  |
| star-halo | 0.1 | look | 0.27, 3% | 0.60, 7% | 0.15 / 0.17 |  |
| grade-stars | 0.6 | look | 0.14, 3% | 0.32, 7% | 0.08 / 0.09 |  |
| nebula-scale | 0.08 | look | 0.21, 1% | 0.08, 1% | 0.00 / 0.00 |  |
| star-halo-deg | 0.4 | look | 0.09, 1% | 0.19, 2% | 0.09 / 0.10 |  |
| thickness | 0.06 | shell style (retiring) | 2.27, 34% | 19.29, 67% | 0.83 / 7.20 | the shell's; in the mass it only sets the march's stride, a quality effect that will need a dial of its own |
| blister | 0.8 | shell style (retiring) | 0.00, 0% | 17.54, 80% | 0.00 / 4.50 | the mass's twin is mass-blister |
| cavity-density | 0.006 | shell style (retiring) | 0.00, 0% | 11.84, 50% | 0.00 / 2.65 | the mass's twin is mass-cavity |
| pillar-width | 0.045 | shell style (retiring) | 0.00, 0% | 8.64, 40% | 0.00 / 0.25 | capsule pillars and clouds, which the mass skips |
| clusters | 2 | shell style (retiring) | 0.00, 0% | 8.64, 49% | 0.00 / 1.60 | the mass's twin is mass-clusters |
| cloud-width | 0.022 | shell style (retiring) | 0.00, 0% | 5.21, 14% | 0.00 / 0.79 | capsule pillars and clouds, which the mass skips |
| cluster-size | 0 | shell style (retiring) | 0.00, 0% | 5.16, 41% | 0.00 / 0.28 | the mass's twin is mass-cluster-size |
| pillars | 9 | shell style (retiring) | 0.00, 0% | 4.87, 23% | 0.00 / 3.23 | capsule pillars and clouds, which the mass skips |
| dust-style | lanes | shell style (retiring) | 0.00, 0% | 3.70, 34% | 1.19 / 2.46 | with dust |
| cavity-spread | 1.7 | shell style (retiring) | 0.00, 0% | 3.34, 28% | 0.00 / 2.28 | with cavity-density |
| clouds | 4 | shell style (retiring) | 0.00, 0% | 3.32, 10% | 0.00 / 0.94 | capsule pillars and clouds, which the mass skips |
| cloud-distance | 0.35 | shell style (retiring) | 0.00, 0% | 3.27, 11% | 0.00 / 1.80 | capsule pillars and clouds, which the mass skips |
| dust-scale | 4 | shell style (retiring) | 0.00, 0% | 2.86, 34% | 0.41 / 1.12 | with dust |
| rim-shadow | 2 | shell style (retiring) | 0.04, 0% | 2.33, 28% | 0.01 / 1.25 | the mass's grazing shadows take its place |
| dust | 0.6 | shell style (retiring) | 0.00, 0% | 2.28, 23% | 0.00 / 0.88 | the mass's twin is mass-dust (off) |
| cloud-length | 0.15 | shell style (retiring) | 0.00, 0% | 2.25, 9% | 0.00 / 0.46 | capsule pillars and clouds, which the mass skips |
| pillar-length | 0.3 | shell style (retiring) | 0.00, 0% | 2.12, 9% | 0.00 / 0.14 | capsule pillars and clouds, which the mass skips |
| dust-opacity | 6 | shell style (retiring) | 0.00, 0% | 1.57, 17% | 0.50 / 1.32 | with dust |
| distant-max-deg | 14 | shell style (retiring) | 0.17, 1% | 1.07, 5% | 0.41 / 0.22 | as distant-count |
| cloud-density | 3 | shell style (retiring) | 0.00, 0% | 0.89, 4% | 0.00 / 0.48 | capsule pillars and clouds, which the mass skips |
| outer-sharpness | 6 | shell style (retiring) | 0.00, 0% | 0.72, 14% | 0.03 / 0.62 | the shell's outer edge, and the distant nebulae's |
| grade-dust | 1 | shell style (retiring) | 0.00, 0% | 0.49, 10% | 0.00 / 0.03 | with dust |
| distant-min-deg | 3 | shell style (retiring) | 0.04, 0% | 0.42, 2% | 0.23 / 0.15 | as distant-count |
| pillar-density | 2.5 | shell style (retiring) | 0.00, 0% | 0.37, 4% | 0.00 / 0.38 | capsule pillars and clouds, which the mass skips |
| distant-count | 3 | shell style (retiring) | 0.26, 1% | 0.28, 1% | 0.23 / 0.17 | the distant nebulae are shells; a mass hides them |
| mass-edge | 0 | off by a decision | 8.99, 62% | 0.00, 0% | 0.99 / 0.00 | hard edges: pending their blind A/B (ROADMAP, Hard edges) |
| mass-dust | 0 | off by a decision | 6.46, 48% | 0.00, 0% | 1.39 / 0.00 | dust in the mass: every kind lost to none, blind, three times |
| spike | 0 | off by a decision | 0.02, 0% | 0.02, 0% | 0.00 / 0.00 | diffraction spikes, off: baked into a sky they read as a telescope's artefact |
| spike-flux | 5000 | off by a decision | 0.00, 0% | 0.00, 0% | 0.00 / 0.00 | which stars get spikes; nothing while spike is off |
| mass-edge-patch | 0 | off by a decision | 0.00, 0% | 0.00, 0% | 0.18 / 0.00 | where the edges are hard; nothing while mass-edge is off |
| young | 40 | no visible effect | 0.02, 0% | 0.04, 0% | 0.00 / 0.01 | the clusters' young stars |
| cluster-stars | 1 | no visible effect | 0.01, 0% | 0.02, 0% | 0.00 / 0.00 | the clusters' lighting stars |
| external-galaxies | 4 | no visible effect | 0.00, 0% | 0.00, 0% | 0.00 / 0.00 | other galaxies, far off |
| nebula | on | debug | 17.25, 81% | 18.50, 58% | -- | the gas off, to judge the stars and galaxy alone |
| grade | auto | debug | 12.51, 89% | 11.55, 93% | -- | physical: the lines' own colours, ungraded, to judge the physics |
| line-colors | natural | debug | 8.94, 65% | 5.79, 45% | -- | the lines' colour mappings; under the grade they only reweight brightness |
| light-res | 96 | quality | 5.56, 53% | 5.19, 35% | -- | trades time for fidelity; studied for cost |
| light-steps | 48 | quality | 4.07, 41% | 2.71, 26% | -- | trades time for fidelity; studied for cost |
| max-steps | 600 | quality | 2.81, 20% | 3.30, 21% | -- | trades time for fidelity; studied for cost |
| step-frac | 0.15 | quality | 1.82, 29% | 2.38, 33% | -- | trades time for fidelity; studied for cost |
| denoise | 0.35 | quality | 0.93, 18% | 1.06, 24% | -- | trades time for fidelity; studied for cost |
| supersample | 2 | quality | 0.54, 8% | 0.78, 13% | -- | trades time for fidelity; studied for cost |
| galaxy-res | 512 | quality | 0.05, 1% | 0.10, 1% | -- | trades time for fidelity; studied for cost |

look: 56, shell style (retiring): 25, off by a decision: 5, no visible effect: 3, debug: 3, quality: 7
