---
sidebar_position: 1
slug: /intro
---

# StarCanopy

**The sky of a place in space, baked for your game.**

StarCanopy generates skyboxes for space games: a nebula of billowing, lit cloud,
the galaxy's band and its stars, around the player, as a cubemap in linear HDR that
any engine can load.

You pick a sky from a seed, look around it at your game's field of view, and steer
it with a handful of controls that describe the sky -- more open or more enveloping,
brighter or more brooding. When it looks right you export it at full size, turned
to face the way your scene needs.

:::note Status
Skies render from the command line: `starcanopy render` turns a project file into
OpenEXR, KTX2 and PNG, steered by thirteen macros. The interface is to come. The
whole is a work in progress.
:::

## What it makes

- **Nebulae with form.** A thick mass of cloud lit by one cluster of young stars, so
  most of it turns from the light and its shape shows; bright ionisation rims; fine
  lumps catching a grazing light; and distant nebulae beyond it, lit from beside,
  dimmed by the dust between.
- **A galaxy seen from inside.** Its band, its dark rift and its spiral arms; its
  stars at real distances, each dimmed by exactly the gas in front of it -- some in
  front of the nebula, some inside, some behind -- and open clusters among them.
  Where in the galaxy you are is part of the sky.
- **Colour from a palette space** fitted to measurements of admired skies: every
  sky is of their kind without copying one.

See the [gallery](gallery) for skies it has made, each with the project file that
makes it again.

## How it is made

Every change to StarCanopy's look is decided by a blind comparison scored by eye,
at a game's field of view -- never by a number or a whole-sky map. The principles
it keeps are in [PRINCIPLES.md](https://github.com/chalkwalk/star-canopy/blob/main/PRINCIPLES.md),
and what it refuses to become in
[NON-GOALS.md](https://github.com/chalkwalk/star-canopy/blob/main/NON-GOALS.md).
