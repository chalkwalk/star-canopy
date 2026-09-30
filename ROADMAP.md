# StarCanopy -- Roadmap

> Named work areas, not numbered ones. A name survives reordering, insertion and
> deletion; a number does not.
>
> For architecture see `DESIGN.md`. For the principles every piece of work must
> satisfy, see `PRINCIPLES.md`, and for the standing refusals `NON-GOALS.md`.
> **Before adding a work area here, confirm it clears both.** Ordering for any new
> work: **PRINCIPLES -> DESIGN -> ROADMAP**.
>
> Completed areas move to `docs/COMPLETED.md` in the commit that finishes them.

## Active focus

*(2026-09-29)*

The model is here and renders from project files to OpenEXR, KTX2 and PNG,
oriented as a scene needs (`docs/COMPLETED.md`); the parameters are triaged and
studied, and twelve macros steer it, every name confirmed blind
(`docs/studies/`). CI waits for a remote to run on. Order from here:

1. ~~**Kick-start**, **the lift**, **headless core and command line**~~ -- done.
2. ~~**Parameter triage and sensitivity study**; **macros**~~ -- done but for
   their ranges and styles (*Macros*).
3. ~~**Retire the shell**~~ -- done in look 2, the distant nebulae remade as
   masses (*The shell's retirement, and bubbles*). Bubbles wait.
4. **The galaxy from any star** -- the vantage atlas first, then dust and a band
   made of stars; the galactic macro and the external galaxies come back with it.
   Done to the observer's place (look 8); the galactic macro's name and routes
   next.
5. **Astronomical objects** -- accents and heroes, open clusters first
   (*Astronomical objects: accents and heroes*). Its place against 6 and 7 is
   the user's to set.
6. **In context** -- headless stills first, then a minimal window and
   look-around to show them in.
7. **The preview ladder** -- a resolution-stable light volume first -- **and the
   rest of the interface.**

---

## Foundations

### Kick-start

- [x] `PRINCIPLES.md`, `NON-GOALS.md`, `DESIGN.md`, `ROADMAP.md`, `README.md`,
      `AGENTS.md` (with `CLAUDE.md` and `GEMINI.md` symlinks), `CONTRIBUTING.md`,
      `THIRDPARTY.md`, GPLv3 `LICENSE`
- [x] CMake build for Linux; SDL3 and Dear ImGui as submodules; glad vendored
- [x] An empty bake that writes a black cubemap, headless (EGL) and windowed
- [x] `docs/references/SOURCES.md`, including the reference-sky measurements'
      provenance (numbers only, `fence #4`)
- [ ] CI on Linux (`.github/workflows/ci.yml` is written; tick this once it has
      run green on the remote); Windows and macOS builds once there is
      something to build
- [ ] `SOURCES.md`'s open item: where the measurement scripts live

## The application

### Preview ladder

- [ ] 128 / 256 / 1024 / export over one pipeline (`PRINCIPLES §9`)
- [x] Measure the light volume's cost at small sizes: at 128 a face it is 3.7 s
      of a mass's 5.1 s bake (`docs/studies/parameters.md`, *Quality*)
- [ ] A light volume that is resolution-stable -- filtered to its voxel, as the
      field's octaves are to the texel -- before any lighter one for previews:
      today 76, 136 or 256 voxels each move the sky from 96's as much as each
      other (a look change: a new look version)
- [ ] A galaxy baked small for small skies: at 128 a face, 64 texels a face of
      glow is indistinguishable from 512; to be measured at export sizes
- [ ] Progressive refinement: a rough frame at once, refined while you wait

### Interface

- [ ] SDL3 + Dear ImGui window
- [ ] Explore: a 5x5 grid at 128, Reroll and Vary
- [ ] Steer: a look-around at game field of view, with compass and axes
- [ ] A/B and side-by-side views; collections of candidates (`PRINCIPLES §1`)
- [ ] Approve at 1024; export dialog
- [ ] Every control named, described, keyboard-reachable (`PRINCIPLES §12`)

### In context

The sky as an engine shows it -- auto exposure, bloom, a tonemap -- with a
stand-in sun at the key light and probes it lights; for looking, never baked
(`DESIGN.md` §11, `fence #8`, `fence #9`). Headless stills come first: they are
testable without a window and give the A/B tools in-context sheets at once.

- [ ] Viewer library (`src/view/`): sky sources (the bake, our exports read back,
      any OpenEXR or `.hdr` sky), IBL precompute (GGX mips, BRDF table, SH9),
      tonemaps, exposure, bloom; tiled against the watchdog
- [ ] Probes (mirror, rough metal, grey diffuse, a simple hull) and the sun
- [ ] `starcanopy view PROJECT|FILE`: headless stills, camera, probe, sun,
      presentation
- [ ] Tests: constant sky, mirror against lookup, BRDF table, export round trip
- [ ] A minimal SDL3 window with the look-around; neutral and in-context views
      as a toggle, the presentation stated on screen (`PRINCIPLES §9`)
- [ ] A/B sheets in context from `tools/`
- [ ] IBL output: KTX2 with the prefiltered mip chain, SH9 in `NAME.json`
      (`DESIGN.md` §8); stb_image in `THIRDPARTY.md` when `.hdr` lands

### Macros

- [x] Triage the ~100 lab parameters: 58 look, 25 retiring with the shell, 5 off by
      a decision, 3 debug, 7 quality, 1 with no visible effect
      (`docs/studies/parameters.md`)
- [x] Sensitivity study: each dial alone over four seeds, the fine ones again at
      512 a face, and Morris's effects for what matters only together and what
      adds up (`docs/studies/parameters.md`; `tools/study`)
- [x] Look by eye at the three dials the study saw change nothing: all three
      implemented. The cluster's own stars and its young stars are points round
      it, invisible to whole-sky measures; the study now has a local one, and
      they are look dials. The external galaxies are smudges a star across, for
      the galaxy work (*Points* in `docs/studies/parameters.md`)
- [x] Macro table (`DESIGN.md` §5) and its resolution, with tests: `[macros]` in
      projects, `render --macro`, `starcanopy macros`
- [ ] First macro set, iterated with blind scoring (`docs/studies/macros.md`):
      thirteen now, galactic confirmed after the galaxy work. Rounds 1 and 2
      confirmed eleven names on every seed; violent
      became turbulent; galactic waits for the galaxy work (*The galaxy from any
      star*); grand was withdrawn, unreadable twice. Billowing, remade with the
      depth of the lumps as its own dial and as large smooth forms, was ranked
      in order on every seed three ways in round 5. Every name now confirmed;
      next, whether the ends' ranges are the ones people want
- [ ] What the macros' preferences say about the defaults, each a look change
      for a new look version and its own blind test: preferred on every seed
      were dim to bright, detailed to smooth, remote to galactic (round 1) and
      clear to hazy (round 2)
- [ ] Styles: mass, shell, and the sparse compositions

## The look

### Hard edges

Soft cloud edges read as indecisive at game field of view (Stephen's in-game
test). Measured, diagnosed and prototyped in the labs: the mass's two density
ramps are the cause; `neb-mass-edge` and `neb-mass-edge-patch` exist, default off.

- [ ] Blind A/B at 45 degrees, 2048 per face (sheets exist, unscored)
- [ ] Depending on the result: harden the billow gate alone; or a seeded field
      with stronger contrast; or hardness that follows the light (crisp lit rims
      and silhouettes, soft faces turned away)

### Sparse and lighter skies

The wrap-around mass is imposing (`PRINCIPLES §11`). Four styles made from existing
dials in the labs -- one region, scattered clouds, a band, a veil -- with their
settings in the labs' `NEXT.md`. Some were made with the shell, retired
in look 2: those are to be found again from the mass.

- [ ] Pick which become styles; make each reliable across seeds (the band only
      works on some)
- [ ] A seed-chosen composition archetype, so sparse skies arrive by browsing too
- [x] Less nebula by default (look 9): open +0.5's dials as the default,
      preferred 4-0 with 4 ties blind (`docs/studies/macros.md`). Close on every
      sky; further, if wanted, another round

### The shoulder in HDR

`DESIGN.md` §4.2 means the shoulder for the 8-bit derivation only; look 1 applies
it in HDR, before the stars, as the look was judged.

- [ ] Blind A/B, then, if it holds up, look 2: HDR unshouldered, the shoulder in
      the 8-bit derivation (stars included)

### The galaxy from any star

The band should emerge from the galaxy's star density as seen from where the
observer is, and look good from any star in it: anywhere in the galaxy's
bounding volume, plus a thin margin for the rare outlier. It already emerges
from a marched 3D density (`DESIGN.md` §3.5), but reads as a smooth glow with a
seam down it. The dust is a thin layer (0.1 kpc, against the stars' 0.3) with
nothing finer than about 250 pc, so near dust only dims broad areas and far dust
draws a line; the glow bakes at 512 a face against an export of 2048; and the
knee is tuned to one vantage. Stars and dust only for now: clusters, emission
knots and satellite galaxies come later.

What the model lacks was read off a photographic all-sky panorama of the Milky
Way, kept outside the repository (`fence #4`): a band made of stars, grainy at
full resolution; dust that breaks it into clouds with windows, black cores and
amber edges, and fingers leaving the plane; stars over the lanes as well as
through them; a band very uneven along its length. It is a guide to what is
missing, not a target. Where look and physics disagree the look wins
(`PRINCIPLES §3`), found by eye and settled blind (`§2`); any number fitted
from it goes in `SOURCES.md`.

- [x] Vantage atlas (`tools/atlas`): eight places, three seeds, the whole sky in
      galactic coordinates and four 45-degree views. Today's model, read in
      `docs/studies/atlas.md`: no band of stars, one smooth line of dust, the
      centre burnt out and the rim empty, no arms from above, no warp. The
      bench for every step below
- [x] Dust across scales (look 3): a lognormal from about 0.8 kpc to 12 pc, the
      finer octaves ridged, clumped more or less by seed; a layer varied in
      thickness and height; the glow baked at the sky's own size in strips;
      the CPU twin in step (`test_galaxy`). Preferred 9-0 blind on the atlas
      (`docs/studies/atlas.md`). The observer's default height is the midplane
- [ ] Softer still than the sky it was read off: windows, black cores and amber
      edges are few. A later pass, judged on the atlas
- [x] The band made of stars (look 4): beyond the field stars, stars where the
      density and luminosity law put them, counted in patches with a tent's
      blend, each dimmed by its own line's dust; the glow only what is too faint,
      with a haze kept (`galaxy-haze`); the bulge flattened; stars scaled from
      the midplane. Preferred 9-0 blind, after a first round's 6-3
      (`docs/studies/atlas.md`)
- [x] Its previews: the band's stars are stars, drawn at every size alike, so a
      small sky has the export's stars, seen more coarsely (`PRINCIPLES §9`)
- [x] Exposure that follows the vantage (look 7): the glow's 90th percentile
      over the sky, measured small, against the Sun's place, followed half way
      (`galaxy-adapt`) within x0.25..x2.5, before the knee. Preferred 7-2 with 3
      ties blind; the centre 3-0 (`docs/studies/atlas.md`). Beyond the disc, the
      stars counted from the disc -- measured from the empty midplane there, the
      band asked for hundreds of millions and ran out of memory
- [x] Arms (look 5): the galaxy's type by seed -- barred, grand-design or
      flocculent spiral; arms carried by knots of young stars that outreach the
      old disc, dark space between them in the outer disc, dust along their
      inner edges, breaking into fragments further out. From above preferred
      3-0 blind, in the disc level (`docs/studies/atlas.md`)
- [x] Lenticulars (look 6), a quarter of galaxies: a large bulge, a lens with a
      sharp edge, a bar in half, stellar rings, a ghost of the spiral, dust from
      none to a few thin rings near the centre. Preferred 12-0 blind
      (`docs/studies/atlas.md`); judged from the atlas's new portrait, 35 kpc
      over the centre
- [ ] Bars that show: the fixed knee burns out the centre where a bar lies, a
      barred spiral's as much as a lenticular's (with *Exposure that follows
      the vantage*)
- [ ] The bulge's size by type, and ellipticals and irregulars
- [ ] Arms that branch into several strands, with spurs and feathers between
- [x] The observer's place (look 8): the seed's own path through the galaxy --
      its place in the band at 0, in along the plane toward +1, out along its
      own route to remote toward -1, from straight out past the edge to straight
      up. Ranges read off a place grid and two mock-ups (`docs/studies/atlas.md`);
      `galaxy-radius` and `galaxy-height` are `auto` unless set
- [ ] The external galaxies: implemented, but smudges a star across that change
      no patch of sky by more than two levels (`docs/studies/parameters.md`,
      *Points*). Make them worth seeing -- resolved discs, a few degrees for the
      nearest -- or drop them. Now *Neighbour galaxies*, under *Astronomical
      objects*
- [x] The galactic / remote macro (look 8, routes look 9): moving the observer
      along the seed's path; its name confirmed on 5 of 6 seeds in each of two
      rounds (`docs/studies/macros.md`). The first one -- nearer the centre, more glow -- lost its name
      0-3: the glow read as a brown haze over the stars. This one, blind, -1 / 0 /
      +1 ranked with the nebula on: in order on 5 of 6 seeds. The sixth reversed:
      its route rose steeply, and 1.2 kpc up the view toward the centre is the
      whole bright disc beneath, lifted by the exposure -- the most galaxy on
      screen. The routes now keep mostly outward (0-45 degrees); in the second
      round remote was least galactic on every seed
- [ ] Starry to move the band too: its star budget (x2 at +1, x1/4 at -1) and
      the haze kept under it the other way, so the glow gives way to stars
      without the galaxy's light changing (the user's proposal)
- [ ] The young arms' reach: from the rim, their star clouds strew much of the
      sky -- striking or clutter, to be judged
- [ ] Blind A/B at game field of view for each step that moves the look, across
      the atlas's vantages, not one; `test_look` moves only with a new look
      version

### The shell's retirement, and bubbles

The mass beat the thin shell 9-1 blind. What the shell alone still gives -- open
sky by default (66-74% of the sky clear against the mass's 11-29%), capsule
pillars and dark clouds, lane dust, the layered teal and red of a front seen
face on -- is either reachable from the mass or better made another way. So the
shell is to be retired completely, not kept by half: as a style, with its
dials, its capsules and its lane dust. What is worth keeping of it comes back as
something new, earning its place blind like anything else.

- [x] ~~Blind A/B: the shell's veil against a translucent mass~~ -- not run: the
      shell was retired whole by decision; lighter skies are for *Sparse and
      lighter skies*, made from the mass
- [x] Retire the shell style (look 2): `form`, 22 shell-only dials, the capsule
      pillars and clouds, and all the nebula's dust -- lanes, veins and the
      mass's own, which lost to none three times -- with the grade's dust ramp.
      The march's stride keeps its per-seed value, no longer a dial (`step-frac`
      scales it). The rim shading the mass drew from the shell's `rim-shadow`,
      up to 18 levels near a cluster, stays, fixed. A mass sky's main nebula is
      as in look 1: 99.997% of texels identical on four seeds, the rest within
      0.03 levels. Look-1 projects are refused, saying how to render them
- [x] Distant nebulae as masses, lit from beside, dimmed by the dust in front:
      four blind rounds against look 1's distant shells, the last two won 4-0
      (`docs/studies/distant.md`)
- [ ] Bubbles, as their own object: now *Glowing shells* under *Astronomical
      objects*, below
- [ ] Pillars and dark clouds that emerge from the mass rather than being
      placed: the light eroding the gas that faces it, leaving the shadowed
      tails behind dense knots pointing at it; dense lumps of the mass seen
      against the glow. Research first (`PRINCIPLES §4`)

### Astronomical objects: accents and heroes

Objects beyond the lit mass and the galaxy -- star clusters, supernova remnants,
planetary nebulae, reflection nebulae, dark clouds, neighbour galaxies -- agreed
in a Q&A with the user (2026-09-29), the list checked against the standard
classes of deep-sky object. Each family gets its own design, spec and plan,
reviewed before it is built, and its own blind rounds.

The decisions, for every family:

- **Both accents and heroes.** An accent is a secondary object, as the distant
  nebulae are; a hero replaces the lit mass as the sky's centrepiece. In a hero
  sky there is no main mass -- distant masses may remain as accents -- and the
  key light is the sky's brightest star (`PRINCIPLES §14`).
- **Auto by seed, explicit otherwise** -- the rule for every choice of kind, as
  `galaxy-style` is: `style = "auto"` lets the seed draw a hero sometimes, so
  browsing turns them up; a named style pins one kind, the seed varying within
  it. About one sky in four a hero under auto, each kind starting rarer while it
  is new, and to be reviewed.
- **Near by choice, adjustable.** Seen from anywhere, most such objects are small:
  a planetary nebula arcminutes, a bright cluster a couple of degrees. Accents
  are drawn at physical distances, sizes and brightnesses, with a bias toward
  near ones -- someone is always near something -- set by a dial from purely
  realistic to favouring the near. Never enlarged beyond what their distance
  allows: that is a decal (`PRINCIPLES §4`).
- **Where the galaxy puts them, not strictly.** Drawn from the galaxy's density,
  so the far ones crowd along the band and are veiled by its dust; the near ones
  lie in any direction, as near things do.
- **Their own colours, pulled toward the sky's.** Physical colours -- a
  planetary nebula's teal core and red rim, a young cluster's blue -- drawn part
  way toward the sky's grade, as the stars are (`grade-stars`).

In this order, each proving the ground the next builds on:

- [x] **Open clusters** (look 10), accents: knots of young stars where the
      galaxy's are, near by `accent-near`, in clear sight, at most 3 degrees
      across, with a textured haze by age. Ten rounds of mock-ups at a game's
      resolution scored by the user; no blind A/B, at the user's direction
      (`docs/studies/accents.md`)
- [ ] Stellar associations, parked: loose young groups tens of parsecs across,
      which from near by dissolve into the field. Back only with a look of
      their own that reads
- [ ] **A Pleiades-like hero**: a near young cluster in a reflection nebula that
      fills the view -- the main nebula's pipeline with its light turned to
      scattering (blue, no glowing gas) and its dust streaked into wisps, lit by
      several bright stars. After the open-cluster accent; its textured haze, or
      the small real nebula that is its fallback, is the start of it
- [ ] **Globular clusters**: hundreds of thousands of old yellow stars in a dense
      ball, in the galaxy's halo -- placed about it, not along the band. Accents,
      and a hero: a sky beside or inside one, bright stars packed on every side
- [ ] **Glowing shells**: supernova remnants and wind-blown bubbles -- thin,
      transparent, limb-brightened, their wrinkles seen edge on as filaments;
      the old shell's geometry with a light of its own, a shock heating the
      whole skin, no light volume -- and planetary nebulae, small, symmetric or
      two-lobed, in layers of colour about a hot central star. Accents first,
      veiled by the dust in front as the distant nebulae are (`Bubble::veil`);
      then heroes, with the style choice and the key light from the brightest
      star. Blind each way
- [ ] **Reflection nebulae**: dust lit by a star near it, blue with scattered
      starlight rather than glowing -- often about a young cluster. The mass lit
      by scattering alone (the `reflection` dial's light, on its own). Accents
- [ ] **Dark clouds and galactic cirrus**: giant molecular clouds, dark nebulae
      and small dense globules, seen against the band and the nebula's glow,
      faintly lit at their edges; and cirrus, faint dust far off the band lit by
      the galaxy's own light, grey-brown wisps over the dark. With the galaxy's
      own dust and *Pillars and dark clouds*
- [ ] **Neighbour galaxies**: dwarfs and irregulars near by, and the odd large
      neighbour -- our own galaxy model seen from outside, as the atlas's
      portrait sees it; this becomes *The external galaxies*. A large neighbour
      can be a hero
- [ ] **Stars of note**, folded into the stars: a supernova -- in a baked sky, one
      extraordinarily bright star, rare -- and colourful ones, deep red carbon
      stars and doubles of contrasting colour. Whenever it is convenient

Folded elsewhere: Wolf-Rayet bubbles and pulsar wind nebulae into the glowing
shells; pillars, cometary globules and small jets into *Pillars and dark
clouds*. Left out: quasars and gravitational lenses (points, unseen at a
skybox's scale); galaxy clusters (smudges, with the neighbour galaxies if at
all); comets, zodiacal light and planets (a star system's, fence #8); black
holes, which the game renders itself.

### Carried over from the labs

- [ ] Faint shadow streaks on some seeds
- [ ] Smooth unlit bulges at a distance (fine lumps near a texel's size)
- [ ] Palette splits by object rather than by a field
- [ ] Adaptive supersampling (estimated 59 s to 25-30 s at 2048)

## Outside this repository

- [ ] A target game that loads HDR skyboxes; a converter for its face order and
      mirroring

## Parked

- A website and wiki, on the model of Antiphon's and Arps Euclidya's.
- Porting the bake to SDL's GPU API, if macOS drops OpenGL.
