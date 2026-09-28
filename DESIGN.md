# StarCanopy -- Design

> Architecture and the reasoning behind it. `PRINCIPLES.md` says what this project
> believes; this file says how those beliefs are built. Sections are cited as
> `DESIGN.md §N` from `ROADMAP.md` and from source comments.
>
> Where a decision was made against a plausible alternative, the alternative is
> recorded with it. A design document that only states the outcome makes the next
> person re-derive the argument, usually badly.
>
> **Status.** The model described in §3-§6 is in `src/core/`, lifted from the Space
> Nerds In Space labs (`labs/features/nebula_sky` on its `nebula-sky` branch),
> where it was developed and judged blind over many rounds; it renders their skies
> to the texel. The application around it -- §2, §7 onward -- is the plan, not yet
> built, but for the headless bake of §2. §11, the in-context view, is planned.

## 1. Vision

A seed and a handful of descriptive controls go in; a cubemap of a nebula, a
galaxy and its stars comes out, in linear HDR, ready for a game engine.

The workflow is a ladder of resolutions over one pipeline (`PRINCIPLES §9`):

| Stage | Size per face | What you do |
|---|---|---|
| Explore | 128 | A grid of small skies. **Reroll** for new seeds; **Vary** to scatter the macros about one you like. Pick one. |
| Steer | 256 | Look around it at game field of view; move the macros; every change re-bakes. |
| Approve | 1024 | Look around at a size that shows edges and detail honestly. |
| Export | 2048 or chosen | The full render, oriented as the scene needs (§8), written in the chosen formats. |

## 2. Architecture

Three layers, so the command line and the interface cannot drift apart:

- **The core** (`src/core/`), C++17: the model, the bake, the preview ladder and
  the writers. It takes a project (§7) and gives a cubemap. It owns an OpenGL 3.3
  core context but no window of its own.
- **The command line** (`starcanopy`): renders a project file, applies
  overrides, writes outputs. Headless: an EGL surfaceless context on Linux, a
  hidden window elsewhere.
- **The interface**: SDL3 for the window, input and context; Dear ImGui over it
  (`imgui_impl_sdl3` and `imgui_impl_opengl3`). The grid, the look-around, the
  macros and the A/B views are ImGui drawing the core's textures.
- **The viewer** (`src/view/`, planned): the in-context view of §11 -- a sky
  presented as an engine would, with a stand-in sun and probes. A library, so
  the interface, the command line (`starcanopy view`) and `tools/` draw the same
  frame (`PRINCIPLES §12`, §13). It reads cubemaps; it never changes one.

**Why OpenGL 3.3 and not Vulkan, WebGPU or SDL's GPU API.** The model is written
in GLSL 1.50 and runs everywhere that matters today, macOS included (4.1 core,
deprecated but present). A port would cost months and buy nothing a user sees.
If macOS removes OpenGL, the bake moves to SDL's GPU API, whose ImGui backend
already exists, and the interface does not change.

**Why the GPU.** At the step counts the look needs, one sky is minutes to hours
on a CPU (`fence #6`).

## 3. The model

Sky units: the main nebula has radius 1 and the viewer is at the origin. Each
nebula ("bubble") is shaded in its own unit frame, so a nebula twenty times
further away and twenty times larger looks identical; only the texel's footprint
changes, and that decides how many octaves of detail are evaluated.

### 3.1 The main nebula: a mass

A thick layer of opaque, billowing cloud from an inner surface out to the bubble's
edge, around a clear pocket the viewer is in. Lobes of the inner surface reach in
over the cavity. Lit by **one** cluster of young stars, so that most of the mass
turns from the light -- which is what gives it form.

- **Lumps:** two octaves of smooth noise for the billows; four finer octaves each
  *displacing the surface* the larger made, so detail is carved into the form.
- **Grazing shadows:** two full-detail samples a lump's width toward the light, so
  small lumps shadow each other where the light grazes (the light volume is far
  too coarse to).
- **Raking fill:** a dim light turned square to the line of sight, shadowed only
  near at hand, so faces the cluster misses show their texture. Not physical;
  kept because it won blind (`PRINCIPLES §3`).
- **Soft shadows:** the cluster sampled from five points turned at random per
  voxel, read back as a power mean (exponent 0.3) of six taps, which keeps the
  sky's mean brightness within a few percent of a point source's.
- **No dust.** Three kinds lost blind to none, and look 2 removed the nebula's dust
  altogether; darkness is to come from the mass itself (`ROADMAP.md`, *The shell's
  retirement, and bubbles*). The galaxy's dust is another matter (§3.5).
- **Rim shading:** points the grazing shadows skip -- almost hidden -- still take
  one coarse sample of the gas's own shadow toward the light, which near a cluster
  is up to 18 levels.

Blind results behind this: the mass beat a thin shell 9-1 ("reads as physical and
tangible"); fine lumps with grazing shadows beat the bare mass 9-1; one cluster
without a glowing cavity scored best of three lightings.

**Open: edges.** At game field of view the mass's edges are soft (a median
boundary 1.6 degrees wide). The cause is its two density ramps, not the lighting;
a dial that narrows them is prototyped, pending a blind test (`ROADMAP.md`,
*Hard edges*).

### 3.2 The distant nebulae

Masses too, some degrees across, seen from outside (`docs/studies/distant.md`).
Lit from within, one glows evenly and has no form, so a distant nebula's clusters
stand beside it, across the line of sight; its gas has no fine lumps (at a few
degrees they are specks), its outline is lobed and squeezed, and a little glow
fills it, rimmed at the edge where its ionisation front is seen edge on. Each
bubble's light is dimmed by the galaxy's dust between it and the viewer
(`Bubble::veil`), scaled up from the thin layer the galaxy's look was tuned with:
a depth cue that applies to anything placed in the sky, the bubbles to come
included.

Look 1 had a second form, a thin folded shell with capsule pillars and dark
clouds, which lost to the mass 9-1 blind; look 2 retired it, and the distant
shells with it.

### 3.3 Light and emission

A light volume per bubble holds the ionising optical depth from each voxel back to
each cluster. Emission is absorbed ionising light re-emitted, so a sharper front
is thinner, not dimmer. [O III], H-alpha and [S II] are weighed by the ionisation
parameter (photons per atom), which keeps teal cores and red rims apart rather
than mixing them to grey. Extinction is integrated exactly over each step.

### 3.4 The march

Rays are intersected with each bubble and marched near to far in each bubble's
frame. Rules that are each the answer to an artefact:

- Long strides through empty space; a long stride that lands in gas steps back and
  re-enters at the fine stride (else a sandy grain on every edge).
- No step spans more than about a third of an optical depth (else pepper).
- Every step length is jittered after every rule that sets it (else contour lines).
- 2x2 supersampling, then an outlier clamp and a bilateral filter.
- Faces are drawn in tiles, and the light volume in strips, so no one draw trips a
  GPU driver's watchdog.

### 3.5 Galaxy and stars

A 3D galaxy model (disc, bar, bulge, arms, warp, bending waves, dust) marched from
where the observer sits, giving the band, its rift and a lopsided sky near the
edge. Its dust is a lognormal of seven octaves, from about 0.8 kpc to 12 pc, the
finer ones ridged into filaments, more or less clumped by seed; octaves finer
than a sample fade out with the mean kept, so a small sky is the same sky
coarser. Its layer varies in thickness and height, so near clouds stand out of
the plane. The glow is baked at the sky's own size (`docs/studies/atlas.md`).
About 30,000 field stars sampled from the same model at real distances, each
dimmed and reddened by exactly the gas in front of it, so stars lie in front of,
inside and behind the nebula. Diffraction spikes are off: baked into a sky they
read as a telescope's artefact.

## 4. The look

### 4.1 Palette space

Every reference sky is a single curve through colour: within a band of lightness
hue hardly varies; chroma arches, peaking in the midtones; hue drifts toward a
yellow-white in the highlights. A palette is a path: a dark hue drawn from one of
four families (warm 17-63 degrees, green 112-152, teal 182-198, blue 228-258, by
weights fitted to the references), turning toward a shared highlight (blue stays
blue). About a third of skies **fan** into a neighbouring hue across part of the
sky. Fitted from measurements only (`fence #4`).

### 4.2 Tone

A faint haze in the palette's darkest colour is added *over* the gas, so no cloud
is darker than empty space; a shoulder eases the brightest channel rather than
clipping it. The shoulder is meant for the 8-bit derivation only, leaving HDR
output unshouldered; the look still applies it in HDR, before the stars, as the
look was judged in the labs. Moving it changes the look, so it waits for a new
look version and a blind comparison (`ROADMAP.md`, *The look*).

## 5. Macros

The surface is a set of macros (`PRINCIPLES §5`). A macro is a named control from
-1 to 1 between two words for the sky -- `open` runs from enveloping to open -- and
0 is the seed's own sky, exactly. Its table of bindings (`src/core/macros.cpp`, the
one definition, `PRINCIPLES §13`) gives each raw parameter it moves two slopes: how
far at 1 and how far at -1, linear between, since a parameter seldom has as much
room one way as the other. A parameter stepped by factors is moved in octaves,
any other by amounts. A parameter's value is its base -- the default, or the
project's override -- plus every macro's amounts, times two to every macro's
octaves, clamped to its range: the additive model Arps Euclidya's macros use, so
one parameter can serve several macros.

Macros bind only look parameters (`docs/studies/parameters.md`): never quality,
debug, or those off by a decision; `test_macros` holds
the table to that. A parameter that exists only to serve one macro -- the
palettes' chroma, vivid's; the depth of the mass's lumps, billowing's -- belongs to
it (`Dial::owner`): it is not a raw
parameter, and is set only through its macro. The first set, made from the
parameter study, is measured and scored blind in `docs/studies/macros.md`; which
way each end should go, and how far, is iterated with blind scoring. Colour family
and hue type stay choices of the seed, overridable.

**Discrete choices are not additive.** The sparse compositions are to be
*styles*; macros steer within a style. Today there is one, the mass.

## 6. Seeds

A seed fixes the composition: bubble placement, clusters, galaxy orientation,
palette family and fan. Every random draw comes from a deterministic generator
derived from the seed, so the same seed is the same sky (`PRINCIPLES §6`, §7).

## 7. Projects and versioning

A project file (TOML) holds: the look version, the seed, the style, macro values,
raw overrides, orientation and outputs. It is small and human-readable.

The **look version** is recorded in every project. Look 1 was the lift from the
labs; look 2 retired the shell and remade the distant nebulae, a mass sky's main
nebula unchanged; look 3 remade the galaxy's dust (`src/core/project.h`). This
StarCanopy renders the latest look, and refuses older ones saying how to render
them with it. A change that alters any sky's
pixels is a new look version; the renderer keeps the old ones renderable, or
refuses with a clear message, rather than silently re-rendering an old project
differently. `test_look` holds each look version to reference statistics of a
few small renders, with tolerances measured across GPUs (the promise between
GPUs is the same sky, not the same bits).

The schema is in `src/core/project.h`; `starcanopy new` writes a commented
example. Unknown keys are errors, since a typo ignored is a different sky.

## 8. Output

- **HDR first** (`PRINCIPLES §8`): linear radiance as OpenEXR (six faces or an
  equirectangular map; half float, ZIP) and a KTX2 cubemap (RGBA16F, linear, one
  level). Half is what the bake stores, so nothing is lost. KTX2 is written by our
  own code, checked with KTX-Software's `ktx validate`.
- **8-bit** PNG derived from the HDR through a stated tonemap -- the display curve
  the look was judged through (`look.h`), per channel, with a positional dither:
  six faces with the common engine naming (`px nx py ny pz nz`), a horizontal
  cross, an equirectangular map. Display-referred; engines that tonemap take HDR.
- **Directions** are stated, not assumed: faces follow the GL cube map convention
  every engine's loader follows, first row at the top; the equirect's mapping is a
  formula (`src/core/sample.h`): centred on +z, +x to the right, +y up.
- **Orientation** (`PRINCIPLES §14`): yaw, pitch and roll applied at export by
  resampling (the identity is an exact copy); the look-around will show a compass
  and axes. The key light's direction -- the main nebula's brightest cluster, turned
  with the sky -- is written beside the images in `NAME.json`, as a lookup vector,
  as azimuth and elevation, and as the way its light travels.
- **Image-based lighting** (planned, an output option): a KTX2 cubemap with a
  GGX-prefiltered mip chain, each mip's roughness stated, and the sky's SH9
  irradiance in `NAME.json` -- what an engine needs to light a scene from the
  sky, made by the same code the viewer uses (§11). The KTX2 writer gains mip
  levels. It is derived from the HDR, not a change to it, but its filter is
  pinned and tested like the look, so it does not drift silently.
- Game-specific layouts (Space Nerds In Space's face order and mirroring, for one)
  are converters, not core formats.

## 9. Performance

Measured in the labs on an AMD laptop integrated GPU, at 2048 per face with 2x2
supersampling: about 59 s for the mass. The view march is 95%
of the GPU time. At 128 per face that pixel count is 256 times smaller; the light
volume's fixed cost (about 4 s for a mass) then dominates, and the preview ladder
will need a lighter light volume at small sizes that still gives the same sky -- to
be measured, not assumed (`PRINCIPLES §9`).

## 10. Testing

- The core's pure parts (seeding, the palette space, macro resolution, the
  galaxy's CPU/GPU twins, writers' round trips) have unit tests.
- The look is tested by eye, blind (`PRINCIPLES §2`), with tools in `tools/` to
  build side-balanced A/B sheets and look-arounds.
- Every look version keeps a small set of reference renders' statistics, so an
  unintended change to the pixels is caught.
- Image-based lighting (§8, §11): a constant sky gives constant irradiance and an
  unchanged prefilter; the mirror probe matches a direct cubemap lookup; the
  BRDF table matches reference values; an exported file read back into the
  viewer matches the bake it came from.

## 11. The in-context view

A sky's HDR is judged through one stated display curve (§4.2, §8); a game shows
it through its own exposure, bloom and tonemap, beside its own sun, lighting its
own ships. The in-context view shows the sky that way, so a developer can judge
it as their scene will present it (`PRINCIPLES §1`, §8) and line their sun up
with the key light (`PRINCIPLES §14`). It is for looking: nothing it adds reaches
an exported file (`fence #8`), and it stays a view, not a scene tool
(`fence #9`).

**The neutral view is the default.** Steering and blind comparisons use the
display curve the 8-bit output is derived through; the in-context view is a
toggle, its presentation stated on screen, and a score made in it says so
(`PRINCIPLES §9`).

### 11.1 Skies it shows

- The live bake, at whatever rung of the preview ladder it is.
- StarCanopy's own exports, read back -- OpenEXR faces or equirect, KTX2 -- which
  checks an export end to end: faces, orientation and the key light in
  `NAME.json`.
- Any other HDR sky, OpenEXR or Radiance `.hdr`, cubemap or equirectangular: a
  game's current skybox, say, to compare against. Shown and nothing more -- no
  macros, no export, no turning but the camera's; never committed (`fence #4`).
  `.hdr` needs stb_image (MIT).

### 11.2 Lighting from the sky

The standard split-sum image-based lighting: a GGX-prefiltered specular mip
chain, a BRDF lookup table, and SH9 irradiance for the diffuse. The same code
makes the IBL output (§8). The prefilter at export size is drawn in tiles, like
the bake, so no draw trips a GPU watchdog.

### 11.3 Probes and the sun

- **Probes**, generated in code, one at a time: a mirror sphere, a rough metal
  sphere, a grey diffuse sphere, and one simple hull shape. The chrome and grey
  balls are look-dev's standard instruments: one shows what the sky reflects,
  the other how much light it gives. No mesh import (`fence #9`).
- **The sun**: a disc, off by default, that starts at the key light's direction,
  with an angular size and an intensity. It lights the probe as a directional
  light as well as showing in the sky. Its default intensity is set relative to
  the sky's key light, so the sky stays visible; a physically bright sun makes
  auto exposure black the nebula out -- true of many engines, worth seeing once,
  a poor default.

### 11.4 Presentation

A few controls, fixed in kind (`PRINCIPLES §5`, `fence #9`):

- **Exposure**: auto, or a manual EV. Auto meters a luminance histogram of the
  frame and ignores the brightest few percent, so a sun or a nebula's core does
  not set the exposure alone.
- **Bloom**: on or off, a fixed downsample-upsample chain.
- **Tonemap**: the display curve (the default, `look.h`), ACES, or AgX.

A still is a function of its inputs: exposure is metered from the frame itself,
with no adaptation over time. The interactive view may ease between meterings;
a still never does.

### 11.5 From the command line

`starcanopy view PROJECT|FILE` renders a still -- camera yaw, pitch and field of
view, probe, sun, presentation -- headless, as the render does. Anything the
interactive view shows, a script can make (`PRINCIPLES §12`), and `tools/`
builds A/B sheets in context from it.

**Alternatives considered.** A sun baked into the texture: a game that adds its
own gets two, and the sky is no longer the dome alone (`fence #8`). Embedding an
engine (Filament, say) for the view: a large dependency and build for a sphere,
a disc and three post passes.
