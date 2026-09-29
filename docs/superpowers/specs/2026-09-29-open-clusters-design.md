# Open clusters: design

*2026-09-29. The first family of `ROADMAP.md`, *Astronomical objects: accents and
heroes*, and the ground every later accent builds on. Agreed section by section
with the user.*

## What it is for

A sky should hold more than one nebula and a galaxy: the objects a real sky has --
clusters, remnants, planetary nebulae -- as accents, secondary to the sky's hero
as the distant nebulae are. Open clusters come first because they are the
simplest to make from what exists (stars at real distances), so the accents'
shared ground -- where they are placed, how near, how veiled -- is proved on
something cheap before the harder objects rely on it.

Success: open clusters that read as open clusters at game field of view --
irregular, sparkling, coloured by their age -- preferred blind in skies with them
against the same skies without; and a placer the next kinds reuse unchanged.

## Decisions carried from the roadmap

- **Accents**, never heroes: an open cluster never replaces the lit mass.
- **Near by choice, adjustable**: physical distances, sizes and brightnesses,
  the draw tilted toward near ones by a dial; never enlarged beyond what the
  distance allows (`PRINCIPLES §4`).
- **Where the galaxy puts them**, not strictly: far ones crowd along the band,
  near ones lie in any direction.
- **Their own colours, pulled toward the sky's**: members are stars, so the
  stars' existing pull toward the grade (`grade-stars`) applies.
- **Told apart from globular clusters**, which come next: open clusters are never
  round, never a glowing ball, never uniformly gold, and live in the disc.

## 1. An open cluster, as drawn

| | |
|---|---|
| Members | a few hundred to a few thousand stars at one distance |
| Size | a core of a few parsecs within about 10-15 pc; the Pleiades are about 2 degrees across from 136 pc |
| Shape | deliberately irregular: a few overlapping lumps, stretched along one axis; never round |
| Age | drawn by seed, a few million years to about a billion |
| Colour | young: a handful of brilliant blue-white stars over many fainter ones; old: the blue gone, yellow-white with a few orange-red giants |
| Remainder | the members too faint to draw are a faint soft glow over the core -- faint, so a cluster reads as a scatter of stars, not a ball |

**Stellar associations** are the loose end of the same: a much larger radius,
few members, young and bright, tens of degrees across when near.

## 2. Placement: the accents' ground

Written as a general placer, not for clusters alone.

- **How many**: a count drawn by seed about a typical number per sky
  (`open-clusters`, about 6; 0 none).
- **Where**: each drawn from the galaxy's young-star density -- the arms and the
  thin disc -- giving a direction and a distance from the observer. So far ones
  crowd along the band, in the arms most; near ones anywhere; and they follow the
  observer along the galactic path, more and brighter in toward the centre, few
  and distant from far out.
- **Near bias**: `accent-near`, 0 purely physical, 1 strongly favouring those
  within a few hundred parsecs; default 0.4. Shared by every accent kind: it is
  about how a sky is composed, not about clusters.
- **Same physics as everything else**: a cluster's angular size is its real size
  over its distance; its members are dimmed and reddened by the galaxy's dust on
  their own lines; the nebula lies in front of it or behind by distance, as for
  every star. Placement ignores the hero: clusters are where the galaxy puts them.

## 3. How it fits the code

- **`src/core/accents.h/.cpp`**, new: `struct Accent` -- its kind, its place in the
  galaxy's frame and as a sky direction and distance, its size, and a seed of its
  own for its details (a cluster's age, richness and shape). `generateAccents(seed,
  galaxy, params)` draws them from a random stream of its own, so no existing
  star, nebula or galaxy draw moves. A later kind adds a kind, not a placer.
- **Members are stars**: `stars.cpp` gains an open-cluster step beside the field,
  band and nebula-cluster stars, turning each accent into members -- the irregular
  spread, the age's mix, the stars' luminosity law, each dimmed by `dustDepth`
  on its own line -- and keeping those bright enough to draw.
- **The remainder is glow**: accents pass to `galaxy.shader` as uniforms, as the
  external galaxies do, up to 16. Where a ray's march passes a cluster's distance
  it adds the cluster's unresolved light there, so the dust in front dims it
  exactly and at no extra cost. No CPU twin: nothing on the CPU uses that light.
- **A limit, to be checked, not assumed**: that glow is part of the galaxy's
  texture, which the main nebula draws over. A cluster in front of the nebula
  keeps its stars in front, carrying their distance, but its faint haze lies
  behind the gas. Faint enough, it should not show; the mock-ups will say.
- **Seeds and looks**: skies change only by gaining clusters -- a new look
  version, with `test_look`'s table recorded again once preferred blind.

## 4. Controls, tests, judging

- **Dials**, raw (`PRINCIPLES §5`, fence #7): `open-clusters` (count, default
  about 6, 0 off) and `accent-near` (0..1, default 0.4). Once seen, the count's
  natural home is `starry`, as its own blind check.
- **`test_accents`**, new:
  - one seed, the same accents; `open-clusters = 0`, none;
  - the count follows the dial;
  - raising `accent-near` brings them nearer on average;
  - the far ones lie near the disc's plane, a thin-disc population;
  - behind a dark lane, a cluster's members are dimmer than a clear one's at the
    same distance;
  - members spread to the cluster's angular size, and no cluster is round.
- **Judged in two steps**: mock-ups, not blind -- a few seeds, physical and
  near-heavy -- for whether they read as open clusters, and whether a cluster's
  haze in front of the nebula shows wrongly; then blind pairs, skies with them
  against without, at game field of view, looking round. Committed as a look
  only if preferred.

## Out of scope here

Globular clusters (next, sharing this placer); reflection nebulosity about young
clusters (*Reflection nebulae*); clusters as part of any macro, until seen.
