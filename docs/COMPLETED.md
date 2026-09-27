# Completed work

The archive. Work areas move here from `ROADMAP.md` once every checkbox in them is
ticked, compressed to the intent, what actually shipped, and any load-bearing
decision made along the way. Withdrawn work is archived too, with the reason.

## Headless core and command line (2026-09-26)

A project file in, a sky out, headless: `starcanopy new` and `starcanopy render`.

- [x] Project file (TOML): look version, seed, style, macros (none yet),
      overrides, orientation, outputs -- unknown keys are errors
- [x] `starcanopy render project.toml`, with `--set name=value` overrides
- [x] HDR output: OpenEXR faces and equirectangular; KTX2 cubemap
- [x] 8-bit PNG derived from HDR: faces, cross, equirectangular
- [x] Orientation: rotation by resampling at export; key-light sidecar
- [x] Look versioning: look 1, a newer look refused, `test_look` pinning it

Load-bearing decisions:

- **KTX2 is our own writer**, not libktx: one uncompressed RGBA16F cubemap is a
  few hundred lines of a documented format. KTX-Software's `ktx validate` accepts
  it, and OpenEXR's own library reads our EXRs back bit-identical to the KTX2.
- **Directions are stated as formulas** (`src/core/sample.h`, README): GL cube
  map faces, an equirect centred on +z with +x to the right, the handedness of
  the faces themselves, and a cross that joins at every seam, tested.
- **The 8-bit tonemap is the display curve the look was judged through**, per
  channel, dithered by position. The shoulder stays in HDR for look 1, against
  `DESIGN.md` §4.2's intent; moving it is a new look version (`ROADMAP.md`).
- **The look is pinned by statistics, not bits**: tolerances measured between the
  Radeon and llvmpipe (0.2% means, 1.8% at the far tail), tight enough that a 2%
  change in brightness fails.

## The lift (2026-09-26)

The model moved from the Space Nerds In Space labs (`labs/features/nebula_sky`,
branch `nebula-sky`, 49 commits) into `src/core/` as a reconstructed history of
fifteen commits, one layer of the finished model each, every one building,
passing its tests and rendering. The first says the history is a
reconstruction; the messages carry what the labs learned, reworded so that no
reference sky is named (`AGENTS.md`).

- [x] Seeded noise and the deterministic generator -- the labs' `mtwist.c`
      vendored unchanged, so seeds are compatible: scenes hash-identical to the
      labs', float for float
- [x] Scene model: bubbles, clusters, pillars, dark clouds, distant nebulae
- [x] The shell, marched; emission by ionisation parameter
- [x] The light volume
- [x] The march's rules (step-back, optical-depth cap, jitter)
- [x] Galaxy and stars, with the density's CPU and GPU twins tested against
      each other
- [x] Palette space, haze, shoulder, and the second palette
- [x] Denoise and supersampling
- [x] The mass, soft shadows, fine lumps, grazing shadows, raking fill
- [x] Correct copyright: no StarCanopy file carries the labs' copied header;
      `mtwist.c`, genuinely Stephen M. Cameron's, keeps his

Left behind on purpose: the comparison star field and the named palettes
(`fence #4`, `docs/references/SOURCES.md`), the SNIS base skybox, and the lab's
debug views.

**How it was checked.** The finished model renders the labs' own skies. On five
skies (seeds 1, 3, 7 as the mass; 7 and 12 as the shell) at 512 a face, at
least 99.997% of texels are bit-identical to the labs', and the rest differ by
one half-float step in one channel. That remainder is the GPU compiler's: with
the labs' own bake shader text in place of the port's -- which drops the SNIS
base skybox and the debug views, dead code at the defaults -- the match is
100%. Speed is unchanged: seed 7 at 2048 a face bakes in 59 s as a mass and
27 s as a shell, as in the labs, on the same integrated Radeon.

**Found on the way.** The same bake twice gave different pixels, by a
half-float step in a scatter of texels: Mesa's radeonsi swaps an optimized
shader in from a background thread mid-bake. The context now asks for
monolithic shaders, and bakes are bit-identical (PRINCIPLES §7); `test_bake`
bakes twice and compares. The labs had the same flaw, unseen.
