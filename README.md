# StarCanopy

**The sky of a place in space, baked for your game.**

StarCanopy generates skyboxes for space games: a nebula of billowing, lit cloud, the
galaxy's band and its stars, around the player, as a cubemap in linear HDR that any
engine can load.

You start from a grid of small skies, pick one, look around it at your game's
field of view, and steer it with a handful of controls that describe the sky --
more open or more enveloping, brighter or more brooding. When it looks right you
export it at full size, turned to face the way your scene needs.

> **Status: skies render from the command line.** The sky model -- developed and
> judged blind over many rounds in the Space Nerds In Space labs -- is here, and
> `starcanopy render` turns a project file into OpenEXR, KTX2 and PNG, steered by
> a first set of macros. The interface is next; see `ROADMAP.md`. Nothing below marked *planned*
> works yet.

## What it makes

- **Nebulae with form.** A thick mass of cloud lit by one cluster of young stars,
  so most of it turns from the light and its shape shows; bright ionisation rims;
  fine lumps catching a grazing light. Or thinner, lighter veils and scattered
  clouds, with open sky between.
- **A galaxy seen from inside.** Its band, its dark rift, and stars at real
  distances, each dimmed by exactly the gas in front of it -- some in front of the
  nebula, some inside, some behind.
- **Colour from a palette space** fitted to measurements of admired skies: every
  sky is of their kind without copying one.

## How you will use it (planned)

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

**Seeing it in context (planned).** A second view shows the sky as an engine
would -- auto exposure, bloom, your choice of tonemap -- with a stand-in sun at
the key light and a chrome, metal or grey probe lit by the sky. It can load your
exports, or any HDR sky, to compare. None of it is baked: the texture is the sky
alone, and your game adds its own sun.

## Requirements (planned)

An OpenGL 3.3-capable GPU. Linux first; Windows and macOS as soon as they build.

## Building

Linux, for now. You need CMake 3.20+, a C++17 compiler, and the development
packages for EGL, OpenGL and X11 or Wayland (SDL3 and Dear ImGui come with the
source, as submodules).

```bash
git clone --recursive <this repository>
cmake -B build
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure
```

## Rendering a sky from the command line

```bash
./build/starcanopy new night.toml --seed 7     # a project, to edit
./build/starcanopy render night.toml           # writes night/ beside it
```

A **project** is how a sky is made again: a small TOML file with the look version
it was made with, a seed, a style (`mass` or `shell`), macros, orientation and
outputs.
`starcanopy new` writes one with every key commented. The same project renders
the same sky; a project from a newer StarCanopy is refused, not rendered
differently.

**Outputs**, listed in the project's `formats`:

| Format | Files | |
|---|---|---|
| `exr-faces` | `NAME_px.exr` .. `NAME_nz.exr` | linear HDR, half float, the faces |
| `exr-equirect` | `NAME_equirect.exr` | linear HDR, equirectangular |
| `ktx2` | `NAME.ktx2` | linear HDR cubemap, RGBA16F, loads into GL or Vulkan as it is |
| `png-faces` | `NAME_px.png` .. `NAME_nz.png` | 8-bit, display-referred |
| `png-cross` | `NAME_cross.png` | 8-bit, a horizontal cross |
| `png-equirect` | `NAME_equirect.png` | 8-bit, equirectangular |

The HDR is the sky itself, unclipped; take it if your engine tonemaps. The PNGs
are derived from it through the display curve the look was designed on.

**Directions.** Faces follow the GL cube map convention every engine's cubemap
loader uses (`px nx py ny pz nz`, first row at the top). The cross is laid out
`-x +z +x -z` across, `+y` above `+z`, `-y` below. The equirectangular map is
centred on `+z`, with `+x` a quarter turn to the right and `+y` up.

**Orientation.** `yaw`, `pitch` and `roll` in the project turn the sky on export:
positive yaw carries `+z` toward `+x`, positive pitch `+z` toward `+y`, positive
roll `+x` toward `+y`. Beside the images, `NAME.json` gives the **key light** --
the light the sky's form is lit by -- as a direction, as azimuth and elevation,
and as `light_travels`: point your scene's directional light that way and it
agrees with the sky.

**Macros.** The controls. Each runs from -1 to 1 between two words for the sky
-- `open` from enveloping to open, `luminous` from brooding to luminous -- and 0 is
the seed's own sky. Set them in the project's `[macros]`, which `new` fills with
every macro at 0, or with `render --macro NAME=VALUE`; `starcanopy macros` lists
them and the dials each one moves. The first set is measured but not yet
judged blind, so its ends may move (`ROADMAP.md`, *Macros*).

**Overrides.** `starcanopy dials` lists the raw parameters; the project's
`[overrides]` and `render --set NAME=VALUE` set them. They are for scripting, not
steering, and they are the base the macros move from: an override of `exposure`
is what `bright` doubles.

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
to its look was decided by blind comparison, scored by eye. The aim was skies of
a quality comparable to EVE Online's, made generatively: their skyboxes informed
only measurements -- how bright, how colourful, at what lightness -- and no
image of theirs is in this repository.

## Licence

**GPLv3.** See `LICENSE`, and `THIRDPARTY.md` for third-party components.
