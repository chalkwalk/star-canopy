---
sidebar_position: 9
title: Command reference
---

## NAME

starcanopy - procedural space skyboxes, baked on the GPU in HDR

## SYNOPSIS

```text
starcanopy new PROJECT.toml [--seed N]
starcanopy render PROJECT.toml [--macro NAME=VALUE]... [--set NAME=VALUE]...
                  [--size N] [--out DIR] [--context K]
starcanopy macros
starcanopy dials
```

## DESCRIPTION

StarCanopy generates skyboxes for space games: a nebula, the galaxy's band and
its stars, as a cubemap in linear HDR. A **project** file says how a sky is made
-- its look version, seed, macros, overrides, orientation and outputs -- and the
same project renders the same sky on any machine. A seed gives the composition;
**macros**, each from -1 to 1 between two words for the sky, steer it.

## COMMANDS

- **starcanopy new** PROJECT.toml [**--seed** N] -- writes a new project with
  every key commented, for seed N (1 if not given). It will not overwrite an
  existing file.
- **starcanopy render** PROJECT.toml [options] -- bakes the project's sky and
  writes it in each of the project's formats, with NAME.json beside the images:
  the key light's direction and what was made.
- **starcanopy macros** -- lists the macros: what each does, and the dials it
  moves.
- **starcanopy dials** -- lists every raw dial with its default and range.

## OPTIONS

For **render**; each one is over what the project says.

- **--macro** NAME=VALUE -- a macro, -1 to 1. Repeatable.
- **--set** NAME=VALUE -- a raw dial's base. Repeatable. For scripting, not
  steering: macros move from it.
- **--size** N -- texels per face.
- **--out** DIR -- where to write.
- **--context** K -- the OpenGL context: **auto** (the default: EGL, else a
  hidden window), **egl** (headless only) or **window** (a hidden SDL window,
  which needs a display).

For **new**:

- **--seed** N -- the project's seed.

## PROJECT FILE

A TOML file. **look** (required) is the look version it was made with; a
project from a newer StarCanopy is refused, not rendered differently. Then:

- **seed**, **style** -- the composition; **mass** is the only style for now.
- **[macros]** -- NAME = VALUE, -1 to 1; 0 is the seed's own sky.
- **[overrides]** -- raw dials, NAME = VALUE: the base the macros move from.
- **[orientation]** -- **yaw**, **pitch**, **roll** in degrees, turning the sky on
  export.
- **[output]** -- **size** (texels per face, default 2048), **directory**
  (default: beside the project file), **name**, **formats**, **equirect_width**.

## OUTPUTS

The formats a project can list:

- **exr-faces** -- NAME_px.exr .. NAME_nz.exr, linear HDR, half float.
- **exr-equirect** -- NAME_equirect.exr, linear HDR.
- **ktx2** -- NAME.ktx2, a linear HDR cubemap, RGBA16F.
- **png-faces** -- NAME_px.png .. NAME_nz.png, 8-bit.
- **png-cross** -- NAME_cross.png, a horizontal cross.
- **png-equirect** -- NAME_equirect.png.

Faces follow the GL cube map convention (px nx py ny pz nz, first row at the
top). The equirectangular map is centred on +z, with +x a quarter turn to the
right and +y up.

## ENVIRONMENT

- **DRI_PRIME=1** -- on Linux with Mesa, render on the dedicated GPU of a
  laptop that also has an integrated one. The **context:** line **render**
  prints names the GPU it got.

## EXIT STATUS

0 on success; 1 if the project cannot be read or a sky cannot be made or
written; 2 if the command line is wrong.

## EXAMPLES

```text
starcanopy new night.toml --seed 7
starcanopy render night.toml
starcanopy render night.toml --size 512 --macro open=0.6 --macro luminous=-0.4
```

## SEE ALSO

The manual, at https://canopy.chalkwalkmusic.com -- the macros, the dials, and
using the outputs in an engine.
