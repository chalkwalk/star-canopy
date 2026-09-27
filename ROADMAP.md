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

*(2026-09-26)*

The model is here: lifted from the Space Nerds In Space labs, it renders their
skies to the texel from the command line (`docs/COMPLETED.md`, *The lift*). CI
waits for a remote to run on. Next is the headless core's real outputs. Order:

1. **Kick-start** -- the build, the licence, the third-party list.
2. ~~**The lift**~~ -- done; `docs/COMPLETED.md`.
3. **Headless core and command line.**
4. **Parameter triage and sensitivity study**, which the macros depend on.
5. **The preview ladder and the interface.**

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

### Headless core and command line

- [ ] Project file (TOML): look version, seed, style, macros, overrides,
      orientation, outputs
- [ ] `starcanopy render project.toml`, with `--set name=value` overrides
      (`starcanopy bake --set` exists, writing PFM, until then)
- [ ] HDR output: OpenEXR faces and equirectangular; KTX2 cubemap
- [ ] 8-bit PNG derived from HDR: faces, cross, equirectangular
- [ ] Orientation: rotation by reprojection at export; key-light direction sidecar
- [ ] Look versioning (`DESIGN.md` §7)

### Preview ladder

- [ ] 128 / 256 / 1024 / export over one pipeline (`PRINCIPLES §9`)
- [ ] Measure the light volume's cost at small sizes; a lighter one only if it
      gives the same sky
- [ ] Progressive refinement: a rough frame at once, refined while you wait

### Interface

- [ ] SDL3 + Dear ImGui window
- [ ] Explore: a 5x5 grid at 128, Reroll and Vary
- [ ] Steer: a look-around at game field of view, with compass and axes
- [ ] A/B and side-by-side views; collections of candidates (`PRINCIPLES §1`)
- [ ] Approve at 1024; export dialog
- [ ] Every control named, described, keyboard-reachable (`PRINCIPLES §12`)

### Macros

- [ ] Triage the ~100 lab parameters: look, quality, debug, vestigial
      (`starcanopy dials` lists them, from the one table in `settings.cpp`)
- [ ] Sensitivity study: sweep each look parameter over seeds; which matter, which
      only matter together, which are dead
- [ ] Macro table (`DESIGN.md` §5) and its resolution, with tests
- [ ] First macro set, iterated with blind scoring
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
settings in the labs' `NEXT.md`.

- [ ] Pick which become styles; make each reliable across seeds (the band only
      works on some)
- [ ] A seed-chosen composition archetype, so sparse skies arrive by browsing too

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
