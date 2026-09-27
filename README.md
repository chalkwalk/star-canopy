# StarCanopy

**The sky of a place in space, baked for your game.**

StarCanopy generates skyboxes for space games: a nebula of billowing, lit cloud, the
galaxy's band and its stars, around the player, as a cubemap in linear HDR that any
engine can load.

You start from a grid of small skies, pick one, look around it at your game's
field of view, and steer it with a handful of controls that describe the sky --
more open or more enveloping, brighter or more brooding. When it looks right you
export it at full size, turned to face the way your scene needs.

> **Status: documents only.** There is no code in this repository yet. The model
> it will use exists, and was developed and judged blind over many rounds in the
> Space Nerds In Space labs; it moves here next. See `ROADMAP.md`. Nothing below
> marked *planned* works yet.

## What it makes (planned)

- **Nebulae with form.** A thick mass of cloud lit by one cluster of young stars,
  so most of it turns from the light and its shape shows; bright ionisation rims;
  fine lumps catching a grazing light. Or thinner, lighter veils and scattered
  clouds, with open sky between.
- **A galaxy seen from inside.** Its band, its dark rift, and stars at real
  distances, each dimmed by exactly the gas in front of it -- some in front of the
  nebula, some inside, some behind.
- **Colour from a palette space** fitted to measurements of admired skies: every
  sky is of their kind without copying one.

## How you use it (planned)

| Stage | Size per face | |
|---|---|---|
| Explore | 128 | A grid of skies. Reroll for new ones, or Vary around one you like. |
| Steer | 256 | Look around at your camera's field of view; move the controls. |
| Approve | 1024 | Look again at a size that shows the detail honestly. |
| Export | 2048+ | HDR (OpenEXR, KTX2) and 8-bit PNG faces, cross or equirectangular. |

Every stage is the same renderer at a different size, so what you approve is what
you get. The command line renders the same project files, for scripting and build
pipelines.

**Orienting it.** Most engines can rotate a skybox, but matching a sky's brightest
light to your scene's sun by eye is fiddly. StarCanopy rotates the sky on export,
shows a compass and axes while you look around, and writes the key light's
direction beside the images.

## Requirements (planned)

An OpenGL 3.3-capable GPU. Linux first; Windows and macOS as soon as they build.

## Documentation

- **`PRINCIPLES.md`** -- the stance, and the gate every proposal passes.
- **`NON-GOALS.md`** -- what StarCanopy refuses to become, and what it offers
  instead.
- **`DESIGN.md`** -- the model and the architecture.
- **`ROADMAP.md`** -- what is next.
- **`AGENTS.md`** -- how to work in this repository.
- **`CONTRIBUTING.md`**, **`THIRDPARTY.md`**.

## How this was built

StarCanopy is written by [ChalkWalk](https://github.com/chalkwalk) in
collaboration with **Claude Opus 5.5**, working against the constraints in
`PRINCIPLES.md` and the standing instructions in `AGENTS.md`, which is checked in
as the honest record of how the work is done.

Its model was developed in the labs of
[Space Nerds In Space](https://github.com/smcameron/space-nerds-in-space), whose
author, Stephen M. Cameron, suggested making it a standalone tool. Every change
to its look was decided by blind comparison, scored by eye, with the skyboxes of
EVE Online as the standard to reach -- used as measurements only; no reference
imagery is in this repository.

## Licence

**GPLv3.** See `LICENSE`, and `THIRDPARTY.md` for third-party components.
