# The macros: the first set, measured

*Study of look 1, 2026-09-27. Tools: `tools/study` -- `study macros` bakes,
`macros.py` makes the tables below; `tools/blind` makes the blind pairs. Data:
`docs/studies/data/macros_mass.csv`.*

The first macro set: twelve controls (fourteen in its first round), each from -1 to 1 between two words for
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
| billowing | wispy | billowing | how deep the mass's lumps are (`mass-billow`, its own), billowed against ridged detail, the edges eroded and swirled or firm |
| crisp | soft | crisp | edge hardness, the clusters' size, the fill's shadows, the ionising opacity |
| turbulent | calm | turbulent | fold, swirl, and how far the far side is blown out |
| luminous | brooding | luminous | the clusters' output, the cavity's glow, the fill |
| bright | dim | bright | exposure, with the stars and the galaxy |
| hazy | clear | hazy | the haze, and the dust's reflection |
| vivid | muted | vivid | the palettes' chroma, from grey to twice their own; the stars and galaxy drawn into the grade |
| starry | sparse | starry | more stars, each dimmer, and further ones still points |

Rules the table keeps, held by `test_macros`:

- **0 is the seed's own sky, bit for bit.** Every macro at 0 resolves to the
  dials as they were; `test_look` does not move.
- **Only look dials.** Never the quality dials, the debug views, the dials off by
  a decision (`mass-edge` waits on its own blind test), those with no visible
  effect, or those retiring with the shell. So the set was the mass's from the
  start, and the shell's retirement in look 2 took nothing from it.
- **Composition is left to the seed** except where a macro needs it: `open` moves
  the viewer. Colour family and hue type stay choices (`DESIGN.md` §5).
- **Two slopes.** Each binding says how far at 1 and how far at -1, since most
  dials have more room one way; the amounts were set so each end is still a sky
  someone might want -- a first guess, which the blind pairs will correct.

One dial is new: `grade-chroma`, a factor on the palettes' chroma. Nothing could
make a sky more colourful than its seed's palette before -- `grade-strength` is at
its maximum by default -- and a vivid / muted macro needs both ways. It is the
vivid macro's own (`Dial::owner`): not a raw parameter, so not listed by
`starcanopy dials` nor settable as an override, only through vivid. At vivid 0 it
is 1 and changes nothing.

## Method

Each macro at -1, -1/2, 1/2 and 1 on eight seeds (1, 3, 5, 7, 11, 13, 17, 19),
the mass, 128 texels a face, every other macro at 0; each bake compared with the
same seed's own sky, and described, as in the parameter study (its *Method*: the
reach in display levels, the share of the sky visibly changed, and the seven
descriptors, none of them checked against blind scores).

A macro's **promise** is the descriptor its name speaks of, where one does: open
is clear sky, dense is opaque sky, detailed and crisp are detail, luminous, bright and galactic are brightness, hazy is less contrast,
vivid is chroma. **Kept on** counts the seeds on which that descriptor rises (or
falls) in order through -1, -1/2, 0, 1/2 and 1. Billowing, turbulent and starry
promise nothing these descriptors see: they are for the eye.

## Tuned by measurement, before the blind pairs

A first table on three seeds kept its promise on eight macros, and
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

The set as it stands after round 5 (below): galactic and grand withdrawn, violent
renamed turbulent, billowing remade; vivid, starry, luminous, hazy and billowing
measured with their current bindings, the others as in the tuning above.

8 seeds. The seeds' own skies: clear 4%-29%, brightness 0.16-0.25.

| macro | reach at -1 / 1 | sky changed at -1 / 1 | from -1 to 1 | promise | kept on |
|---|---|---|---|---|---|
| open | 18.4 / 20.2 | 86% / 87% | bright+(8/8) contr+(6/8) clear+(8/8) opaque-(8/8) chroma+(6/8) detail+(8/8) top10-(8/8) | clear+ | 5/8 in order |
| dense | 8.8 / 6.2 | 73% / 62% | bright-(8/8) contr-(7/8) clear-(8/8) opaque+(8/8) detail+(8/8) top10+(5/8) | opaque+ | 8/8 in order |
| fragmented | 7.9 / 13.8 | 62% / 79% | bright+(8/8) clear+(8/8) opaque-(8/8) detail+(8/8) top10-(7/8) | clear+ | 6/8 in order |
| detailed | 8.4 / 7.3 | 70% / 67% | bright-(8/8) contr-(7/8) clear-(8/8) detail+(8/8) top10-(6/8) | detail+ | 8/8 in order |
| crisp | 5.5 / 4.4 | 65% / 55% | contr+(8/8) detail+(8/8) top10+(8/8) | detail+ | 8/8 in order |
| turbulent | 5.0 / 13.6 | 55% / 77% | bright+(6/8) contr+(6/8) clear+(8/8) opaque-(8/8) detail+(7/8) | -- | by eye |
| bright | 11.4 / 19.1 | 85% / 91% | bright+(8/8) contr+(8/8) chroma+(8/8) detail+(8/8) top10+(8/8) | bright+ | 8/8 in order |
| luminous | 7.8 / 13.2 | 65% / 75% | bright+(8/8) contr+(8/8) chroma+(8/8) detail+(8/8) | bright+ | 8/8 in order |
| hazy | 4.4 / 24.6 | 89% / 100% | bright+(8/8) contr-(8/8) chroma+(8/8) detail-(8/8) top10-(8/8) | contr- | 8/8 in order |
| vivid | 6.5 / 5.3 | 82% / 82% | bright+(5/8) contr-(8/8) chroma+(8/8) detail-(8/8) | chroma+ | 8/8 in order |
| starry | 0.9 / 2.5 | 9% / 21% | bright+(8/8) contr+(8/8) detail+(8/8) top10+(8/8) | -- | by eye |
| billowing | 11.9 / 12.4 | 79% / 75% | contr+(6/8) clear+(8/8) opaque-(4/8) detail-(6/8) top10+(6/8) | -- | by eye |

The skies at the ends, over the seeds (min-median-max):

| macro | clear at -1 | clear at 1 | brightness at -1 | brightness at 1 |
|---|---|---|---|---|
| open | 2%-7%-14% | 9%-38%-58% | 0.11-0.14-0.17 | 0.22-0.25-0.27 |
| dense | 6%-20%-34% | 3%-14%-25% | 0.19-0.24-0.29 | 0.15-0.17-0.23 |
| fragmented | 3%-9%-19% | 13%-24%-39% | 0.14-0.18-0.24 | 0.18-0.22-0.26 |
| detailed | 4%-18%-32% | 3%-14%-25% | 0.17-0.22-0.29 | 0.16-0.20-0.25 |
| crisp | 4%-17%-30% | 4%-16%-28% | 0.19-0.20-0.24 | 0.17-0.21-0.25 |
| turbulent | 4%-16%-29% | 12%-26%-41% | 0.16-0.20-0.26 | 0.17-0.22-0.24 |
| bright | 4%-16%-29% | 4%-16%-29% | 0.11-0.14-0.19 | 0.24-0.29-0.34 |
| luminous | 4%-16%-29% | 4%-16%-28% | 0.13-0.16-0.21 | 0.22-0.27-0.32 |
| hazy | 4%-16%-29% | 4%-16%-29% | 0.12-0.16-0.22 | 0.30-0.32-0.37 |
| vivid | 4%-16%-29% | 4%-16%-29% | 0.16-0.20-0.23 | 0.16-0.20-0.29 |
| starry | 4%-16%-29% | 4%-16%-29% | 0.16-0.20-0.25 | 0.17-0.21-0.26 |
| billowing | 2%-8%-13% | 7%-23%-51% | 0.15-0.21-0.24 | 0.17-0.20-0.24 |

Macros that move the descriptors alike (cosine over the seven, from -1 to 1, over 0.8):

- bright and luminous: +0.97
- crisp and starry: +0.96
- open and fragmented: +0.95
- fragmented and turbulent: +0.95
- detailed and starry: +0.94
- open and turbulent: +0.89
- detailed and crisp: +0.85

## Reading it

- **Seven macros keep their promise in order on every seed:** dense, detailed,
  crisp, bright, luminous, hazy, vivid. Open and fragmented keep its direction on
  every seed and its order on five and six of eight: where a sky is already nearly
  closed, enveloping has little left to close (seed 11 sits so deep in its mass
  that open takes it only from 4% clear to 9%), and fragmented saturates at its top
  on two.
- **Billowing makes the sky less detailed**, on six seeds of eight, and moves
  how open it is, 8% clear at wispy to 23% at billowing by the median: flattened
  lumps close the mass into a sheet, deep ones leave gaps between them. That may
  be part of what billows are; the blind sheets will say whether it reads as
  billowing or as open.
- **The ends are skies, not voids.** Open reaches 9-58% clear at 1 and 2-14% at
  -1; no other macro moves the median clear share outside 7-26%; brightness stays
  within 0.11-0.37, where the seeds' own skies span 0.16-0.25.
- **The ends are uneven.** Hazy reaches 25 levels at 1 and 4 at -1 -- there is
  little haze to take away -- and bright, luminous, turbulent and starry reach
  further at 1 than at -1 too. Some of that is the display curve, which shows
  brightening more than dimming, and some is room.
- **Macros alike to these descriptors are not the same macro.** Seven numbers
  cannot tell crisp from starry (both add contrast and detail), nor luminous from
  bright (both brighten). Round 1 told each of those apart by name on every seed.

## The blind pairs

`tools/blind` renders, for each macro and seed (3, 7 and 12), the sky at -1 and
at 1, each seen at 75 degrees across, 16:9, toward its key light and turned away
from it; one sheet a pair, the two sides drawn at random, the pairs shuffled, the
key in a file of its own. The scorer says which side is more as the macro's name
says, and which they would rather have. The sheets are written outside the
repository, like every render.

A name is confirmed when the scorer picks its side on every seed; a preference
for one end on every seed says the macro's range, or the default, wants moving.
With `--around`, each side is seen six times, smaller, every 60 degrees round the
horizon: for what shows only as one looks around.

## Round 1, blind

Fourteen macros on seeds 3, 7 and 12, the two ends of each, 42 pairs, scored
2026-09-27. *Named*: pairs where the scorer picked the side the name says.
*Rather have*: + the macro's end, - its opposite, = neither, by seed.

| macro | named | rather have (3, 7, 12) | the scorer's notes, in short |
|---|---|---|---|
| open | 3/3 | = = + | |
| dense | 3/3 | = + = | |
| fragmented | 3/3 | = - + | neither crop very fragmented |
| detailed | 3/3 | + + + | "probably too detailed, but the less detailed version looks a bit strange" |
| crisp | 3/3 | = + - | |
| luminous | 3/3 | = - = | "extremely luminous, though the other is rather too dark" |
| bright | 3/3 | - - - | |
| hazy | 3/3 | = - = | "I was expecting a more overall haze" |
| vivid | 3/3 | - = = | "reads like saturation (which is okay)" |
| starry | 3/3 | + - - | too many stars at their brightness: "striking", "a little excessive" |
| violent | 3/3 | - - = | "I don't really know what violent means"; "could look cool" |
| grand | 1/3 | = - - | not obvious in a crop, "rather once you look around"; how does it differ from enveloping? |
| billowing | 1/3 | - = + | the crops very similar; "hard to understand what billowing vs wispy does" |
| galactic | 0/3 | - - - | how is it different from starry? |

**Eleven names confirmed** on every seed. Of the three that were not:

- **Galactic lost its name 0-3**, the remote end picked as the more galactic every
  time, and preferred every time. Seen, the galactic end's glow of unresolved stars
  lies over the sky as a brown haze and softens the stars; the remote end is black
  sky and sharp stars, which is what reads as a galaxy. So nearer-and-brighter is
  not galactic while the band is a glow. The macro is withdrawn until the band is
  made of stars (`ROADMAP.md`, *The galaxy from any star*), where galactic can mean
  the galaxy's structure seen.
- **Grand and billowing at 1/3 are chance**, as the notes say: at 75 degrees two
  crops of one sky rarely show the size of its forms or its billows against its
  filaments. Both are re-tested looking around. Grand also still opened the sky --
  the scorer asked how it differs from enveloping -- through the mass's own scale,
  which it no longer binds.
- **Violent was named 3/3 but not understood.** Its difference showed, its meaning
  did not. Re-tested looking around, and a candidate for a better name.

**The ends, from the notes:** starry's many stars were too bright together, so
starry now dims each star as it adds them; luminous went too far both ways, and is
shorter both ways; hazy's veil was too slight, and goes further. Vivid read as
saturation, which it is, and runs now from grey at -1 to twice the palette's own
chroma at 1, with the stars and galaxy drawn into the grade at both ends, so a
muted sky is muted all through.

**Preferences on every seed** -- dim over bright, detailed over smooth, remote over
galactic -- say something of the defaults rather than of the macros: the sky may
want a lower exposure, more detail, and less of the galaxy's glow. Each would be a
look change, for a new look version and a blind test of its own (`ROADMAP.md`,
*Macros*); nothing here changes a default.

## Round 2, blind

Seeds 5, 11 and 19, none seen before. Vivid, starry, luminous and hazy with their
ends moved, as in round 1, 12 pairs; grand, billowing and violent looking around
(`--around`), 9 pairs. Scored 2026-09-27.

| macro | named | rather have (5, 11, 19) | the scorer's notes, in short |
|---|---|---|---|
| starry | 3/3 | + + = | |
| luminous | 3/3 | - = - | |
| hazy | 3/3 | - - - | |
| vivid | 3/3 | - + = | |
| violent | 3/3 | = - - | |
| billowing | 3/3 | = - + | "doesn't read as billowing"; "perhaps the action is not strong enough? A scale problem?"; "not obvious at either end" |
| grand | 0/3 | = = = | "still unclear to me what this means", every pair |

- **Starry, luminous, hazy and vivid keep their names** with their ends moved. The
  scorer would rather have the clear end of hazy on every seed, as the dim end of
  bright in round 1: a sky with less over it. Another note for the defaults.
- **Violent is renamed turbulent**, at the scorer's word: named every time in both
  rounds, never understood.
- **Grand is withdrawn.** Looking around, the busy end was picked as the grand
  one on every seed. Seen, grand's few large forms leave whole views empty between
  them, and the busy end has structure everywhere -- which is what reads as grand.
  Two rounds, two ways of seeing it, and the scorer could not say what it meant: a
  control people cannot read is not one to keep (`PRINCIPLES §5`). What it held --
  the scale of folds, holes and detail -- stays in the dials, and in fragmented and
  detailed.
- **Billowing was named, but barely seen.** The billows' depth was fixed in the
  shader; the macro could only trade billowed detail for ridged. It now has a dial
  of its own, `mass-billow`, the depth of the mass's lumps (1 as judged, so
  `test_look` does not move), owned by billowing like vivid's chroma: deep rounded
  lumps with firm edges at 1, flattened ones eroded into ridged, swirled strands at
  -1. Two versions were rejected by eye before any pair was drawn: fine lumps on
  the billows crumple them rather than round them, and larger deep lumps pull the
  mass apart into clumps in clear sky, a second open. Round 3 tests it looking
  around, on seeds 2, 23 and 29.

## Round 3, blind

Billowing alone, remade, looking around, seeds 2, 23 and 29. **Named 3/3**; rather
have: = = + (by seed 29, 23, 2). The scorer: clearer now, still a little weak --
push it further -- and a wish to see the other end against the seed's own sky.

So billowing goes further both ways: deeper lumps, more ridged and eroded edges,
more swirl at wispy, and the lumps' size a little further apart. And `tools/blind`
gains `--three`: the sky at -1, 0 and 1 in an order drawn at random, A, B and C
from the left, put in order from least to most by the scorer -- still blind, and
the two ends seen against the seed's own. Round 4 is billowing that way, on seeds
31, 37 and 41.

## Round 4, blind, three ways

Billowing at -1, 0 and 1 on each sheet, looking around, seeds 31, 37 and 41.
Ranked least to most billowing, the three came out **in exactly the reverse order
on every seed**: 1, 0, -1. The scorer would rather have 1, 0 and 0. On two sheets
the -1 sky was far from the other two, on one the 1 sky: the steps are not even.

So pushed further, the wispy end -- flattened lumps smoothed into swirled veils --
became what reads as billowing, and the billowing end -- deep lumps with all their
fine detail and firm edges -- read as clumps. The scorer's suggestion fits it: more
billow wants less detail. Billowing now draws the fine detail out of its deep lumps
and softens their edges (less detail gain, fewer fine lumps, fewer grazing
shadows); wispy breaks flatter lumps into fine, ridged, eroded filaments and loses
the swirl that read as billowing; and wispy's step is shorter than before, as it
was the far one twice. Round 5 tests it three ways, on seeds 43, 47 and 53.

## Round 5, blind, three ways

Billowing remade as large smooth forms, at -1, 0 and 1, looking around, seeds 43,
47 and 53. **Ranked in order on every seed**, -1, 0, 1 from least to most
billowing; rather have: 0, 0, -1 (by seed 53, 43, 47). No sheet was noted as
having one sky far from the other two. Billowing is confirmed: the twelfth name
of the set, which now holds every one of its names on every seed it was scored
on.

