---
sidebar_position: 6
---

# Outputs, and using them

The project's `formats` lists what `render` writes:

| Format | Files | |
|---|---|---|
| `exr-faces` | `NAME_px.exr` .. `NAME_nz.exr` | linear HDR, half float, the faces |
| `exr-equirect` | `NAME_equirect.exr` | linear HDR, equirectangular |
| `ktx2` | `NAME.ktx2` | linear HDR cubemap, RGBA16F, loads into GL or Vulkan as it is |
| `png-faces` | `NAME_px.png` .. `NAME_nz.png` | 8-bit, display-referred |
| `png-cross` | `NAME_cross.png` | 8-bit, a horizontal cross |
| `png-equirect` | `NAME_equirect.png` | 8-bit, equirectangular |

The HDR is the sky itself, unclipped: take it if your engine tonemaps. The PNGs
are derived from it through the display curve the look was designed on.

## Directions

Faces follow the GL cube map convention every engine's cubemap loader uses (`px nx
py ny pz nz`, first row at the top). The cross is laid out `-x +z +x -z` across,
`+y` above `+z`, `-y` below. The equirectangular map is centred on `+z`, with `+x`
a quarter turn to the right and `+y` up.

## Orientation and the key light

`yaw`, `pitch` and `roll` in the project's `[orientation]` turn the sky on export:
positive yaw carries `+z` toward `+x`, positive pitch `+z` toward `+y`, positive
roll `+x` toward `+y`.

Beside the images, `NAME.json` gives the **key light** -- the light the sky's form
is lit by -- as a direction, as azimuth and elevation, and as `light_travels`:
point your scene's directional light that way and it agrees with the sky. The sky
has no sun in it; your game adds its own.
