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

*(2026-09-27)*

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
5. **In context** -- headless stills first, then a minimal window and
   look-around to show them in.
6. **The preview ladder** -- a resolution-stable light volume first -- **and the
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
      twelve now. Rounds 1 and 2 confirmed eleven names on every seed; violent
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
Full notes: `labs/features/nebula_sky/NEXT.md` in Space Nerds In Space.

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
- [ ] The band made of stars: stars near enough to meet the nebula stay points
      in 3D; beyond them the march counts expected stars per solid angle in a
      few apparent-brightness bins, each reddened by the dust at its depth, and
      a pass at export resolution draws them from a hash fixed to the sky. Only
      what is fainter than the last bin stays glow, and the sky between stays
      dark (`PRINCIPLES §11`)
- [ ] Its previews: stars defined in angle; at small sizes a texel sums the
      bright bins' stars and takes the expected light of the faint ones -- the
      same sky seen more coarsely (`PRINCIPLES §9`)
- [ ] Exposure that follows the vantage, in place of the fixed knee
      (`GAL_KNEE`), so the centre does not burn out and the outer sky is not
      empty
- [ ] Arms that break into fragments, with dust along their inner edges -- if
      the atlas shows the views from above need it
- [ ] The observer's place, cylindrical and normalised: an angle round the disc;
      a radius from 0 at the centre to 1 at the edge; a height from 0 at the
      midplane to +-1 at the disc's top and bottom; a little beyond 1 for
      outliers. The seed picks it, weighted toward the star density but not
      strictly, so most seeds sit in the band and few far out. Today the seed
      picks only the angle, and radius and height are raw dials. Raw material
      for a macro (*Macros*)
- [ ] The external galaxies: implemented, but smudges a star across that change
      no patch of sky by more than two levels (`docs/studies/parameters.md`,
      *Points*). Make them worth seeing -- resolved discs, a few degrees for the
      nearest -- or drop them
- [ ] A galactic / remote macro, once the band is made of stars. The first one
      -- nearer the centre, more glow -- lost its name 0-3 blind: the glow read
      as a brown haze over the stars, and the remote end, black sky and sharp
      stars, was picked as the more galactic every time (and preferred). A
      galactic sky should be one where the galaxy's structure shows
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
- [ ] Bubbles, as their own object: a supernova remnant or wind-blown bubble --
      thin, transparent, limb-brightened, its wrinkles seen edge on as
      filaments. The shell's geometry is right for it and its light is not: a
      remnant glows because a shock heats its whole skin, not because a cluster
      inside ionises one side of it. A self-glowing skin, no light volume. Placed
      at any depth, veiled by the dust in front as the distant nebulae are
      (`Bubble::veil`) -- ahead of the main mass bright and unveiled, far off
      dim. Blind: a mass sky with one against one without
- [ ] Pillars and dark clouds that emerge from the mass rather than being
      placed: the light eroding the gas that faces it, leaving the shadowed
      tails behind dense knots pointing at it; dense lumps of the mass seen
      against the glow. Research first (`PRINCIPLES §4`)

### Carried over from the labs

- [ ] Faint shadow streaks on some seeds
- [ ] Smooth unlit bulges at a distance (fine lumps near a texel's size)
- [ ] Palette splits by object rather than by a field
- [ ] Adaptive supersampling (estimated 59 s to 25-30 s at 2048)

## Outside this repository

- [ ] Space Nerds In Space loads HDR skyboxes; a converter for its face order and
      mirroring

## Parked

- A website and wiki, on the model of Antiphon's and Arps Euclidya's.
- Porting the bake to SDL's GPU API, if macOS drops OpenGL.
