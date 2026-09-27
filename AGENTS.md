# AGENTS.md -- StarCanopy orientation notes

## What this project is

**StarCanopy** generates skyboxes for space games: a nebula, a galaxy and its
stars, baked on the GPU into a cubemap in linear HDR. A seed gives the
composition; macros with descriptive names steer it; a preview ladder (128, 256,
1024, export) runs one pipeline at four sizes. C++17, CMake, OpenGL 3.3, SDL3 and
Dear ImGui. GPLv3.

Authoritative docs (read these before designing anything new):

- **`PRINCIPLES.md`** -- fourteen principles every change must satisfy, with a
  proposal gate at the top. Cited as `PRINCIPLES §N`; the numbers are stable
  anchors.
- **`NON-GOALS.md`** -- the standing refusals, and what we offer instead. Cited as
  `fence #N`.
- **`DESIGN.md`** -- the model and the architecture.
- **`ROADMAP.md`** -- named work areas with checkboxes.
- **`docs/references/SOURCES.md`** (planned) -- where each model and fitted
  constant came from.

Ordering for any new work: **PRINCIPLES -> DESIGN -> ROADMAP**. If a proposal
cannot be expressed within the principles, it is not ready for the roadmap.

## Status

`ROADMAP.md` is the live source; update that file, not this one, when focus
changes. As of 2026-09-26 the model is lifted (`docs/COMPLETED.md`): `starcanopy
bake` renders the labs' skies to the texel, as PFM faces; project files, real
output formats, macros and the interface are not built yet.
The model lives in Space Nerds In Space, `labs/features/nebula_sky` on the
`nebula-sky` branch (`/home/programming/space-nerds-in-space`), and its open work
is in `labs/features/nebula_sky/NEXT.md` there.

## The things most easily got wrong

**1. Judging the look by a number, or by a whole-sky map (`PRINCIPLES §1`,
`§2`).** A change to the look ships only after a blind comparison scored by the
human, at game field of view. A candidate that matched the references' tone bands
far better than the default lost 10-0. A metric is quoted only with whether it has
been checked against blind scores.

> Blind means: labelled pairs, each variant on the left in half of them,
> shuffled; the key written to a file and not opened until the scores are in.

**2. A preview that is not the render (`PRINCIPLES §9`).** Speed comes from
resolution. A cheaper algorithm at preview size steers the user by a sky they
will not get.

**3. Parameters leaking into the interface (`PRINCIPLES §5`, `fence #7`).** Raw
parameters are command-line overrides. A new control must first fail to be a
macro.

**4. Reference imagery in the repository (`PRINCIPLES §10`, `fence #4`).**
Reference skies are used as numbers only. No image of one is ever committed,
including in docs, fixtures or history. Nor are they named: the game they come
from appears once, in `README.md`, and never in code, parameters, docs or commit
messages; no palette or parameter says which reference sky it was fitted to.
The labs' commit messages name it freely -- rewrite them in the lift.

**5. GPU watchdogs.** A single long draw hangs the desktop's GPU driver and
resets it. Faces are drawn in tiles and the light volume in strips for this
reason; keep any new pass split the same way.

## Layout map

Entries marked *(planned)* do not exist yet.

```
CMakeLists.txt
cmake/             # build helpers: shaders embedded into the binary
extern/            # SUBMODULES: SDL3, Dear ImGui; glad vendored (generated)
src/core/          # the model, the bake, the writers -- no window, no UI
src/core/shaders/  # GLSL 1.50: field, light, bake, denoise, galaxy, stars
src/cli/           # starcanopy: project file in, images out
src/app/           # SDL3 + Dear ImGui interface (planned)
tools/             # A/B sheets, look-around capture, measurements (planned)
test/              # unit tests for the pure parts; bake tests on a real context
docs/references/   # SOURCES.md: provenance of models and fitted numbers
```

## Canonical commands

```bash
git submodule update --init --recursive
cmake -B build
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure
./build/starcanopy bake --size 512 --set seed=7 --out DIR # until render exists
./build/starcanopy dials                                 # the raw parameters
./build/starcanopy render sky.toml                         # planned
```

The bake tests need an OpenGL 3.3 context: EGL headless on Linux, which Mesa's
software renderer can provide, so they run without a display or GPU access
(slowly). The hidden-window test skips when there is no display.

The model's shaders are the labs' GLSL, lightly edited; keep them ASCII, and keep
the GPU and CPU twins (`galaxy.glsl` / `galaxy.cpp`) in step -- `test_galaxy`
checks.

## Working conventions

- **Commit as you go**, one logical change per commit, each building and working.
- **Stage files by name.** Never `git add -A`, `git add .` or `git commit -a`.
- Commit messages: a one-line imperative summary describing the effect, then the
  why. End with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Do not push; the human pushes.
- Style: C++17, 2-space indent, braces on the same line, lowerCamelCase members,
  `#pragma once`, namespace `starcanopy`. Comments say why, not what.
- Cite principles and fences by number in source comments where a decision turns
  on one.
- No licence header or SPDX line in our own files: `LICENSE` (GPLv3) and
  `README.md` cover the tree, as in Antiphon and Arps Euclidya. Vendored and
  lifted third-party files, `mtwist.c` among them, keep their own notices.

## Before claiming a change works

- `cmake --build build` clean, `ctest` green -- **quote the output**.
- If it changes the look: where is the blind comparison, and what did it score?
- If it quotes a number about the look: does that measure track blind scores?
- If it adds a control: why is it not a macro?

## Documentation maintenance

Each file at the root has one job; put a fact in exactly one of them.

| File | Owns |
|---|---|
| `AGENTS.md` | How to work in this repo. Map and rulebook, not reference. |
| `PRINCIPLES.md` | Why. Stable `§N` anchors, cited from everywhere. |
| `NON-GOALS.md` | What we refuse, and what we offer instead. `fence #N`. |
| `DESIGN.md` | What the software is. Stable `§N` anchors. |
| `ROADMAP.md` | What it is becoming. Named work areas, checkboxes. |
| `README.md` | The user manual. |
| `CONTRIBUTING.md` | How an outside contributor works here. |
| `THIRDPARTY.md` | Component licences and their obligations. |

`CLAUDE.md` and `GEMINI.md` are **symlinks** to this file; they are tracked and
should stay symlinks.

- **Ship the doc change with the code change.** A finished work area moves to
  `docs/COMPLETED.md` in the commit that finishes it.
- **`PRINCIPLES §N` and `fence #N` are stable anchors.** A renumber sweeps every
  reference in the docs and in `src/` in the same change.
- **Keep `README.md` honest.** It may run ahead of reality only for items clearly
  marked planned.
