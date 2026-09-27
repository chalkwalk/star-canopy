# The macros: the first set, measured

*Study of look 1, 2026-09-27. Tools: `tools/study` -- `study macros` bakes,
`macros.py` makes the tables below; `tools/blind` makes the blind pairs. Data:
`docs/studies/data/macros_mass.csv`.*

The first macro set: fourteen controls, each from -1 to 1 between two words for
the sky, made of the look dials the parameter study found
(`docs/studies/parameters.md`). What each binds, and how far, is in
`src/core/macros.cpp` and printed by `starcanopy macros` -- the one definition
(`PRINCIPLES §13`); this study does not repeat it.

This measures whether each macro moves the sky the way its name says, as far as
a number can see it. It does not say that the name is right, nor which end is
better: that is the blind pairs' to say (`PRINCIPLES §1`, `§2`), and until they
are scored the set is a proposal.

## The set

| macro | -1 | 1 | made of |
|---|---|---|---|
| open | enveloping | open | where the viewer stands in the mass, its inner surface and lobes, the holes kept |
| dense | tenuous | dense | the gas's density, and the mass's |
| fragmented | whole | fragmented | column-density contrast, erosion, the holes' size and share |
| detailed | smooth | detailed | the detail octaves' gain and start, the mass's fine lumps and their grazing shadows |
| billowing | wispy | billowing | billowed against ridged detail, and a little of the mass's scale |
| crisp | soft | crisp | edge hardness, the clusters' size, the fill's shadows, the ionising opacity |
| grand | busy | grand | the scale of the mass, its folds, holes and detail |
| violent | calm | violent | fold, swirl, and how far the far side is blown out |
| luminous | brooding | luminous | the clusters' output, the cavity's glow, the fill |
| bright | dim | bright | exposure, with the stars and the galaxy |
| hazy | clear | hazy | the haze, and the dust's reflection |
| vivid | muted | vivid | the palettes' chroma (`grade-chroma`, new), the stars' and galaxy's grade |
| starry | sparse | starry | the stars' count, brightness and reach |
| galactic | remote | galactic | nearer the galaxy's centre, and its glow |

Rules the table keeps, held by `test_macros`:

- **0 is the seed's own sky, bit for bit.** Every macro at 0 resolves to the
  dials as they were; `test_look` does not move.
- **Only look dials.** Never the quality dials, the debug views, the dials off by
  a decision (`mass-edge` waits on its own blind test), those with no visible
  effect, or those retiring with the shell. So the set is the mass's; the shell
  reads only the dials the two share.
- **Composition is left to the seed** except where a macro needs it: `open` moves
  the viewer, `galactic` the observer's place in the galaxy. Colour family and
  hue type stay choices (`DESIGN.md` §5).
- **Two slopes.** Each binding says how far at 1 and how far at -1, since most
  dials have more room one way; the amounts were set so each end is still a sky
  someone might want -- a first guess, which the blind pairs will correct.

One dial is new: `grade-chroma`, a factor on the palettes' chroma. Nothing could
make a sky more colourful than its seed's palette before -- `grade-strength` is at
its maximum by default -- and a vivid / muted macro needs both ways. At 1 it
changes nothing.

## Method

Each macro at -1, -1/2, 1/2 and 1 on eight seeds (1, 3, 5, 7, 11, 13, 17, 19),
the mass, 128 texels a face, every other macro at 0; each bake compared with the
same seed's own sky, and described, as in the parameter study (its *Method*: the
reach in display levels, the share of the sky visibly changed, and the seven
descriptors, none of them checked against blind scores).

A macro's **promise** is the descriptor its name speaks of, where one does: open
is clear sky, dense is opaque sky, detailed and crisp are detail, grand is less
detail, luminous, bright and galactic are brightness, hazy is less contrast,
vivid is chroma. **Kept on** counts the seeds on which that descriptor rises (or
falls) in order through -1, -1/2, 0, 1/2 and 1. Billowing, violent and starry
promise nothing these descriptors see: they are for the eye.

## First round, and what it changed

A first table on the first three seeds kept its promise on eight macros, and
showed three faults, fixed before the round below:

- **Vivid went the wrong way at its muted end.** It also lowered `grade-strength`,
  which hands the colour back to the lines' own -- and those are more colourful
  than most palettes, not less. Muted is now `grade-chroma` alone; vivid keeps its
  promise on every seed.
- **Grand and billowing were a second open.** Both leaned on `mass-scale`, whose
  largest effect is not the billows' size but how much sky they leave clear: grand
  took the median seed from 8% clear to 33%. Their share of it is smaller now.
- **Luminous, galactic and hazy went much further up than down** (reach 36, 28
  and 16 levels at 1, against 9, 8 and 4 at -1). Their upper slopes are shorter.

## What each macro does

8 seeds. The seeds' own skies: clear 4%-29%, brightness 0.16-0.25.

| macro | reach at -1 / 1 | sky changed at -1 / 1 | from -1 to 1 | promise | kept on |
|---|---|---|---|---|---|
| open | 18.4 / 20.2 | 86% / 87% | bright+(8/8) contr+(6/8) clear+(8/8) opaque-(8/8) chroma+(6/8) detail+(8/8) top10-(8/8) | clear+ | 5/8 in order |
| dense | 8.8 / 6.2 | 73% / 62% | bright-(8/8) contr-(7/8) clear-(8/8) opaque+(8/8) detail+(8/8) top10+(5/8) | opaque+ | 8/8 in order |
| fragmented | 7.9 / 13.8 | 62% / 79% | bright+(8/8) clear+(8/8) opaque-(8/8) detail+(8/8) top10-(7/8) | clear+ | 6/8 in order |
| detailed | 8.4 / 7.3 | 70% / 67% | bright-(8/8) contr-(7/8) clear-(8/8) detail+(8/8) top10-(6/8) | detail+ | 8/8 in order |
| billowing | 11.8 / 11.1 | 76% / 74% | bright-(5/8) contr-(7/8) clear+(4/8) opaque-(3/8) detail-(5/8) | -- | by eye |
| crisp | 5.5 / 4.4 | 65% / 55% | contr+(8/8) detail+(8/8) top10+(8/8) | detail+ | 8/8 in order |
| grand | 17.2 / 15.9 | 87% / 83% | bright-(5/8) clear+(5/8) detail-(7/8) | detail- | 3/8 in order |
| violent | 5.0 / 13.6 | 55% / 77% | bright+(6/8) contr+(6/8) clear+(8/8) opaque-(8/8) detail+(7/8) | -- | by eye |
| luminous | 10.0 / 24.1 | 67% / 82% | bright+(8/8) contr+(8/8) chroma+(8/8) detail+(8/8) | bright+ | 8/8 in order |
| bright | 11.4 / 19.1 | 85% / 91% | bright+(8/8) contr+(8/8) chroma+(8/8) detail+(8/8) top10+(8/8) | bright+ | 8/8 in order |
| hazy | 4.4 / 11.1 | 89% / 100% | bright+(8/8) contr-(8/8) chroma+(8/8) detail-(8/8) top10-(8/8) | contr- | 8/8 in order |
| vivid | 3.9 / 5.3 | 67% / 82% | bright+(5/8) contr-(8/8) chroma+(8/8) detail-(8/8) | chroma+ | 8/8 in order |
| starry | 0.9 / 4.3 | 9% / 28% | bright+(8/8) contr+(8/8) detail+(8/8) top10+(8/8) | -- | by eye |
| galactic | 6.6 / 11.5 | 42% / 50% | bright+(8/8) contr+(8/8) detail-(8/8) | bright+ | 8/8 in order |

The skies at the ends, over the seeds (min-median-max):

| macro | clear at -1 | clear at 1 | brightness at -1 | brightness at 1 |
|---|---|---|---|---|
| open | 2%-7%-14% | 9%-38%-58% | 0.11-0.14-0.17 | 0.22-0.25-0.27 |
| dense | 6%-20%-34% | 3%-14%-25% | 0.19-0.24-0.29 | 0.15-0.17-0.23 |
| fragmented | 3%-9%-19% | 13%-24%-39% | 0.14-0.18-0.24 | 0.18-0.22-0.26 |
| detailed | 4%-18%-32% | 3%-14%-25% | 0.17-0.22-0.29 | 0.16-0.20-0.25 |
| billowing | 5%-14%-23% | 6%-19%-46% | 0.16-0.21-0.25 | 0.17-0.20-0.24 |
| crisp | 4%-17%-30% | 4%-16%-28% | 0.19-0.20-0.24 | 0.17-0.21-0.25 |
| grand | 7%-12%-21% | 5%-18%-40% | 0.15-0.20-0.23 | 0.15-0.19-0.21 |
| violent | 4%-16%-29% | 12%-26%-41% | 0.16-0.20-0.26 | 0.17-0.22-0.24 |
| luminous | 4%-16%-29% | 4%-16%-28% | 0.11-0.15-0.19 | 0.26-0.32-0.37 |
| bright | 4%-16%-29% | 4%-16%-29% | 0.11-0.14-0.19 | 0.24-0.29-0.34 |
| hazy | 4%-16%-29% | 4%-16%-29% | 0.12-0.16-0.22 | 0.24-0.26-0.31 |
| vivid | 4%-16%-29% | 4%-16%-29% | 0.16-0.20-0.24 | 0.16-0.20-0.29 |
| starry | 4%-16%-29% | 4%-16%-29% | 0.16-0.20-0.25 | 0.18-0.22-0.27 |
| galactic | 4%-16%-29% | 4%-16%-29% | 0.12-0.17-0.23 | 0.20-0.26-0.30 |

Macros that move the descriptors alike (cosine over the seven, from -1 to 1, over 0.8):

- crisp and starry: +0.97
- luminous and bright: +0.96
- open and fragmented: +0.95
- fragmented and violent: +0.95
- billowing and grand: +0.94
- detailed and starry: +0.94
- detailed and grand: -0.93
- grand and starry: -0.90
- open and violent: +0.89
- billowing and starry: -0.87
- billowing and crisp: -0.86
- detailed and crisp: +0.85
- crisp and grand: -0.83
- detailed and billowing: -0.83

## Reading it

- **Eight macros keep their promise in order on every seed:** dense, detailed,
  crisp, luminous, bright, hazy, vivid, galactic. Open and fragmented keep its
  direction on every seed and its order on five and six of eight: where a sky is
  already nearly closed, enveloping has little left to close (seed 11 sits so deep
  in its mass that open takes it only from 4% clear to 9%), and fragmented
  saturates at its top on two.
- **Grand makes the sky less detailed on seven seeds of eight, but not in order.**
  Detail here is a texel's difference from its neighbours at 128 a face, which
  sees fine structure, not the size of forms; whether grand gives few large forms
  is for the eye.
- **The ends are skies, not voids.** Open reaches 9-58% clear at 1 and 2-14% at
  -1; no other macro moves the median clear share outside 9-26%; brightness stays
  within 0.11-0.37, where the seeds' own skies span 0.16-0.25.
- **The ends are uneven.** Luminous, bright, hazy, violent, galactic and starry
  still reach further at 1 than at -1, starry most (4.3 levels against 0.9: fewer
  stars change a small share of the sky). Some of that is the display curve --
  brightening shows more than dimming -- and some is room: calm starts near the
  defaults. Whether the ends should be even is a question for the pairs, as seen.
- **Macros alike to these descriptors are not the same macro.** Seven numbers
  cannot tell crisp from starry (both add contrast and detail), nor luminous from
  bright (both brighten). Those pairs are the ones to watch in the blind scores: a
  pair the scorer cannot tell apart by name is a pair to merge or redesign.

## The blind pairs

`tools/blind` renders, for each macro and seed (3, 7 and 12), the sky at -1 and
at 1, each seen at 75 degrees across, 16:9, toward its key light and turned away
from it; one sheet a pair, the two sides drawn at random, the pairs shuffled, the
key in a file of its own. The scorer says which side is more as the macro's name
says, and which they would rather have. The sheets are written outside the
repository, like every render.

A name is confirmed when the scorer picks its side on every seed; a preference
for one end on every seed says the macro's range, or the default, wants moving.
Results go here when scored.
