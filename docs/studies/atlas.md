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

## The band made of stars (look 4)

The band was a smooth glow behind stars as dense off it as on it. Now, beyond the
field stars' 1.5 kpc, the sky is counted in patches, 48 a side on each cube
face: along each, out through the galaxy's density and dust, the stars bright
enough to draw -- the same luminosity law as the field stars, their number per
unit of light carried on from them -- and about 150,000 drawn (`band-stars`),
each dimmed by the dust along its own line, so the lanes are short of stars. The
glow keeps only the light of the stars too faint to draw. They are stars like
the field stars, drawn at every size alike, so a preview has the export's stars.
88% of them lie within 10 degrees of the plane.

**Round 1, blind:** 6-3 for the band. The scorer found the bulge odd in both --
a round ball of glow above and below the band, which no photograph of the Milky
Way shows -- found the old glow's haze compelling, and saw diagonal sawteeth along
the band's edge. The saw was the patches: each held a count, spread evenly in it,
so the count stepped where the density changes fast; a tent two patches wide now
blends them. The bulge is flattened, half as deep as wide, and weaker. And a
share of the glow is kept wherever stars are drawn, 0.3 (`galaxy-haze`): the
stars drawn are a budget, and a real sky has countless fainter ones.

**Round 2:** preferred 9-0, unscored sheet by sheet. From above the disc the
band's stars then blazed white: their number per unit of light was taken from the
field stars' ball about the observer, nearly empty 3 kpc up. Both are now scaled
from the midplane under the observer, and fewer field stars lie about an
observer out of the disc, as few do (`test_stars`).

**Still wanting:** the galaxy has no form but a disc and its noise -- no arms from
any vantage, as the atlas found at the start. The far side, seen from above, is
there but pressed toward the horizon by perspective: from 0.5 kpc up, the disc
15 kpc away lies 2 degrees below it.


## Arms, and the galaxy's type (look 5)

The galaxy's type is now drawn by seed: a barred spiral, a grand design of two
strong arms, or a flocculent spiral of many short ones (and a lenticular, by
name only; below). Judged from the Sun's place and from 3 kpc above, over seeds
1, 7, 11 and 16.

**Round 1** was not scored: from above, a field of soft round bumps covered the
disc and swamped the arms. They were not physical -- the old disc's light was
clumped by up to a factor of two over a kiloparsec, and its dust's coarsest
octave was 0.8 kpc. An old disc is smooth: its stars have had billions of years
to mix. What is clumpy is the young and the dust. So the old light varies by
30% at most, the young stars come in knots, and the dust starts at 0.4 kpc.

**Round 2** had clear arms but, the scorer said, flat: even along their length.
**Round 3** varied them -- width, strength, segments, a meander -- and was
preferred 2-1 with five ties, but the scorer found round 2's arms much clearer:
the variation filled the space between them.

Photographs of face-on and edge-on spirals, viewed outside the repository
(`fence #4`) for structure only, showed what was missing:

1. **Dead space between the arms.** In the outer disc the arms are separate
   strands with near-dark gaps; ours had half the arms' light between them.
2. **The arms are drawn by young stars**, beaded with knots of clusters.
3. **The arms outlast the old disc**, carrying on after its light has faded.
4. **The inner disc is smooth**, with thin dust lanes along the arms.
5. **Bulges vary with the type**, from nearly all bulge to a small core.
6. **Arms branch** into several strands, and wind one or two turns.

The first three are made: round 2's clarity back, width varying by a fifth at
most; in the outer disc the old light between arms falls to a third; the young
stars sharper along the arms, in knots, on a radial scale 1.6 times the old
disc's. The last two wait (`ROADMAP.md`).

**Round 4, blind:** from above, the new arms preferred 3-0; from the Sun's place
two ties and two for look 4. The scorer judged the in-disc views all good,
differing little, the view from above the differentiator. One of the look 4
wins was seed 7 drawn as a lenticular -- a spiral's disc with the arms taken
out, which read as neither -- and the other had bright young knots piling up
into blotches along the band.

**The lenticular** is a disc galaxy that has stopped making stars, not a spiral
wound tight: its arms fade with their young stars. What it has instead is
concentric: a bulge far larger than a spiral's; a lens, a plateau of even light
with a sharp edge, in nearly all of them, about 1.3 times a bar's length; a bar
in many; stellar rings at the bar's size and twice it; a spiral surviving only as
a trace; little dust, often in lanes or rings round the centre. So it is not
drawn by seed until it has that structure; its quarter of seeds goes to the
spirals in proportion, every other seed keeping its type.

## Lenticulars (look 6)

Made as the sources describe them: a bulge 1.6 to 2.4 kpc in scale, against a
spiral's 1, and rounder; a lens, a plateau of old light ending in a sharp edge,
1.3 times the bar where there is one; a bar, strong, in half; an inner stellar
ring in half and an outer one at twice its radius in three of ten; the spiral a
trace at 1 to 3%; the old disc's lumpiness scaled down with it; and dust in six
of ten, then only in thin rings about the centre, from none in the rest.

Judged from a new vantage, the *portrait*, 35 kpc over the centre: the galaxy
whole, looking down, as photographs show one -- beyond any sky's reach. From
there the glow's march had drawn the disc in concentric rings, its steps spread
over the empty way in until each was as thick as the disc; it now starts where
the ray enters the galaxy. The old lenticular's ghost spiral, wound tight, had
shown the same rings, and that was real: it went to 1-3%.

**Blind:** the new lenticular preferred **12-0** over look 5's, from the
portrait, from 3 kpc up and from the Sun's place, on seeds 1, 3, 5 and 7. A
lenticular is now drawn for a quarter of seeds. Two things wait: bars barely
show, the fixed knee burning out the centre where they lie (a barred spiral's
too); and the ring dust, seen from above, is specks rather than lanes.

## Exposure that follows the vantage (look 7)

The fixed knee was set at one vantage. Measured (`atlas --measure 1`), the glow's
brightest part -- the core, its 99.9th percentile -- is about as bright from
anywhere, surface brightness not falling with distance; what swings is how much
of the sky the galaxy fills. Its 90th percentile over the sky, seeds 1, 7, 12:

| vantage | p90 | p99.9 |
|---|---|---|
| centre | 1.0 - 1.9 | 2.1 - 5.5 |
| mid-disc | 0.17 - 0.42 | 2.7 - 12 |
| sun | 0.09 - 0.29 | 2.2 - 12 |
| above-low | 0.11 - 0.27 | 4.1 - 11 |
| above-high | 0.10 - 0.25 | 2.4 - 4.9 |
| rim | 0.005 - 0.07 | 2.4 - 12 |
| past-edge | 0.0006 - 0.026 | 1.6 - 9.6 |
| outlier | 0.0003 - 0.012 | 3.2 - 5.9 |
| portrait | ~0 | 0.25 - 0.71 |

So the glow is exposed by (0.12 / p90) to the power `galaxy-adapt`, 0.5 by
default, within x0.25 and x2.5, before the knee: about x0.3 at the centre, 1 at
the Sun's place, up to x2.5 at the rim and beyond -- the cap so a small bright
galaxy far off is not blown out finding light in the dark.

**Blind:** preferred **7-2**, three ties, over the fixed knee: the centre 3-0,
mid-disc 2-0, the rim and past the edge 1-1 each. Both losses were the
lenticular far out, its large bulge lifted; the first thing to lower if that
recurs is the cap.

Measuring past the edge found a fault: the stars' number per unit of light was
taken from the midplane under the observer, which there is empty -- every speck
of the halo's light worth billions of stars, and the band asked for hundreds of
millions, ran for hours and out of memory. It is now taken no further out than
four scale lengths; the limit's search bisects where its slope steps swing; the
draw is capped at twice the budget (`test_stars`).
