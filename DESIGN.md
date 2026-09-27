# StarCanopy -- Design

> Architecture and the reasoning behind it. `PRINCIPLES.md` says what this project
> believes; this file says how those beliefs are built. Sections are cited as
> `DESIGN.md §N` from `ROADMAP.md` and from source comments.
>
> Where a decision was made against a plausible alternative, the alternative is
> recorded with it. A design document that only states the outcome makes the next
> person re-derive the argument, usually badly.
>
> **Status.** The model described in §3-§6 exists and was developed in the Space
> Nerds In Space labs (`labs/features/nebula_sky` on its `nebula-sky` branch),
> where it was judged blind over many rounds. The application around it -- §2, §7
> onward -- is the plan, not yet built.

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

### 3.1 The main nebula: a mass (the default)

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
- **No dust.** Three kinds lost blind to none.

Blind results behind this: the mass beat a thin shell 9-1 ("reads as physical and
tangible"); fine lumps with grazing shadows beat the bare mass 9-1; one cluster
without a glowing cavity scored best of three lightings.

**Open: edges.** At game field of view the mass's edges are soft (a median
boundary 1.6 degrees wide). The cause is its two density ramps, not the lighting;
a dial that narrows them is prototyped, pending a blind test (`ROADMAP.md`,
*Hard edges*).

### 3.2 The shell (the other form)

A thin folded sheet: a radial profile read at a point displaced by noise, so folds
seen edge on become filaments; holes; a lognormal column density so most sight
lines are thin and a few thick; pillars pointing at their cluster; dark clouds in
the cavity, backlit. Kept as a form for veils and lighter skies (`PRINCIPLES §11`).

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
edge. About 30,000 field stars sampled from the same model at real distances, each
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
clipping it. In HDR output (§8) the shoulder belongs to the 8-bit derivation only.

## 5. Macros

The surface is a set of macros (`PRINCIPLES §5`). A macro is a named control in
[0, 1] (or bipolar) and a table of bindings; each binding is a raw parameter, an
intensity and a curve. A parameter's value is its base plus the sum of every
macro's contribution, clamped to its range -- the additive model Arps Euclidya's
macros use, so one parameter can serve several macros.

The macro set is **not yet designed**. It comes after a triage of the raw
parameters and a sensitivity study (`ROADMAP.md`), and is iterated with blind
scoring. Candidates from what is known: *open / enveloping*, *billowing / wispy*,
*luminous / brooding*, *calm / violent*, and colour family.

**Discrete choices are not additive.** The form (mass or shell) and the sparse
compositions are *styles*; macros steer within a style.

## 6. Seeds

A seed fixes the composition: bubble placement, clusters, galaxy orientation,
palette family and fan. Every random draw comes from a deterministic generator
derived from the seed, so the same seed is the same sky (`PRINCIPLES §6`, §7).

## 7. Projects and versioning

A project file (TOML) holds: the look version, the seed, the style, macro values,
raw overrides, orientation and outputs. It is small and human-readable.

The **look version** is recorded in every project. A change that alters any sky's
pixels is a new look version; the renderer keeps the old ones renderable, or
refuses with a clear message, rather than silently re-rendering an old project
differently.

## 8. Output

- **HDR first** (`PRINCIPLES §8`): linear radiance as OpenEXR (six faces or an
  equirectangular map) and KTX2 cubemaps.
- **8-bit** sRGB PNG derived from the HDR through a stated tonemap: six faces with
  the common engine naming (`px nx py ny pz nz`), a horizontal cross, an
  equirectangular map.
- **Orientation** (`PRINCIPLES §14`): a rotation applied at export by reprojection;
  the look-around shows a compass and axes. The key light's direction is written
  beside the images (a small JSON sidecar) so a scene's light can be matched.
- Game-specific layouts (Space Nerds In Space's face order and mirroring, for one)
  are converters, not core formats.

## 9. Performance

Measured in the labs on an AMD laptop integrated GPU, at 2048 per face with 2x2
supersampling: about 59 s for the mass, 26 s for the shell. The view march is 95%
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
