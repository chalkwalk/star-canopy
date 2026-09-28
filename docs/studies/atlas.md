# The vantage atlas: the galaxy as look 2 makes it

*Look 2, 2026-09-28. Tool: `tools/atlas`. The sheets are kept outside the
repository; `atlas --out DIR` makes them again.*

The first step of *The galaxy from any star* (`ROADMAP.md`): the galaxy and its
stars, the nebula off, from eight fixed places over seeds 1, 7 and 12, 1024
texels a face. Each sheet shows the whole sky in galactic coordinates -- the
galaxy's centre in the middle, its plane across, its north up -- and four views 45
degrees across: toward the centre, away from it, along the plane, and toward the
pole on the disc's side, down onto it from above. It is the bench each step of the
galaxy work is judged on; this is what the model makes today.

| vantage | radius, scale lengths | height, kpc | what it is |
|---|---|---|---|
| centre | 0.5 | 0.03 | in the bulge |
| mid-disc | 2.5 | 0.03 | inside the Sun's radius |
| sun | 3.0 | 0.03 | the Sun's place |
| rim | 5.0 | 0.03 | near the disc's edge, at 5.5 |
| past-edge | 6.0 | 0.03 | just beyond it |
| above-low | 3.0 | 0.5 | out of the disc a little |
| above-high | 3.0 | 3.0 | well above it |
| outlier | 6.5 | 1.5 | the margin a rare seed may reach |

## What emerges

- **In the disc (mid-disc, sun)** the galaxy is a band with a dark rift and a bulge
  toward the centre -- the right parts, in the right places. Its shape varies by
  seed as it should.
- **Above it (above-low)** half the sky is the disc as a floor of glow, the band
  its horizon, the bulge rising behind: the right geometry.
- **From the rim and beyond (rim, past-edge, outlier)** the galaxy draws in to a
  bright-cored disc seen edge on, some tens of degrees across.
- **The external galaxies** show, at this size, as small soft ellipses -- more
  than the whole-sky measures said (`parameters.md`, *Points*), and still small.

## Where it breaks

In order of how much of the sky, and how many seeds, each touches:

1. **The band is not made of stars.** At every vantage in the disc the stars are
   as dense off the band as on it -- the field stars are the ones within 1.5 kpc,
   near enough to lie all round -- and the band is a smooth glow behind them. It is
   the first thing wrong in the view most skies will have. (*The band made of
   stars*.)
2. **The dust is one smooth, soft line.** The rift is a blurred tube with no
   clouds, windows or amber edges; from above, the dust is soft blotches some
   hundreds of parsecs across and nothing finer; seen along the plane it is a
   single stripe. The galaxy's glow is baked at 512 a face, and the dust has no
   scale under about 250 pc. (*Dust across scales*.)
3. **Near the centre the sky burns out** to an even grey fog, the rift a smudge
   across it: the fixed knee, tuned to one vantage, compresses a sky that is
   bright everywhere into flatness. From the rim the opposite: the galaxy is a
   small bright core and the rest of the sky is empty, the local disc all round
   too faint to show. (*Exposure that follows the vantage*.)
4. **There are no spiral arms from anywhere.** Looking straight down from 3 kpc
   shows soft dust and even glow; the arms, their young stars and their dust, do
   not read at all. The roadmap made arms that break into fragments conditional
   on the views from above needing them: they do, though they are the rarest
   vantage. (*Arms that break into fragments*.)
5. **The warp and the bending waves** do not show at any vantage: the band is flat.

The first two are in every sky a seed makes at the default vantage (radius 3.5,
between *sun* and *rim*); the third in the few seeds that sit near the centre or
the edge; the fourth only above the disc. The roadmap takes the dust before the
band of stars, and should: the band's stars are reddened by the dust at their
depth, so the dust comes first.

## Dust across scales (look 3)

The dust made again (`ROADMAP.md`): a lognormal of seven octaves from about 0.8
kpc to 12 pc -- most of the volume thin, a few clouds dense -- whose octaves finer
than a sample fade out with the mean kept; the finer ones ridged, so it makes
filaments with edges rather than soft round blobs; a layer that varies in
thickness and height, so near clouds stand out of the plane; 512 steps, and the
glow baked at the sky's own size, in strips. A galaxy at 2048 a face takes 18 s.

Two versions were seen and set aside by eye on the way: smooth noise at a spread
of 1.6, which broke the rift into clumps but left it soft; and at 2.4, clumpier,
still soft. Softness was the shapes, not the resolution -- at 45 degrees across a
texel at 2 kpc is 3 pc -- and ridging the finer octaves gave the rift edges and
wisps.

**Blind, on the atlas** (`tools/atlas/pairs.py`; mid-disc, sun and above-low on
seeds 1, 7 and 12, the nebula off): **the new dust preferred 9-0.** The scorer
picked the more detailed on every pair but asked for the range of possibilities
rather than always the most dramatic dust: so the lognormal's spread is drawn by
seed, 1.2 to 2.6, from a soft haze of lanes to a dense cloud complex. And
noticed fewer stars above the plane than below: physical, the observer being the
default 30 pc above a disc 300 pc thick -- a fifth more stars below -- but not
what a sky should have without anyone choosing it, so the default height is now
the midplane. Places above and below come back with the seed's choice of place
(*The observer's place*). The atlas and its pairs show the whole sky in the
Equal Earth projection, at the scorer's preference.

