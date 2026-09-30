---
sidebar_position: 2
---

# Getting started

## Build it

StarCanopy is tested on Linux; it builds on Windows and macOS, but they are
unproven. You need CMake 3.20 or newer, a C++17 compiler, an OpenGL 3.3-capable
GPU, and on Linux the development packages for EGL, OpenGL and X11 or Wayland.
SDL3 and Dear ImGui come with the source, as submodules.

```bash
git clone --recursive https://github.com/chalkwalk/star-canopy.git
cd star-canopy
cmake -B build
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure
```

## Make a first sky

```bash
./build/starcanopy new night.toml --seed 7     # a project, to edit
./build/starcanopy render night.toml           # writes night/ beside it
```

`night/` then holds the sky in every format the project lists -- HDR faces, a KTX2
cubemap, PNGs -- and `night.json` with the direction of its key light. Change the
seed for a different sky, or move a [macro](macros) to steer this one:

```bash
./build/starcanopy render night.toml --macro open=0.6 --macro luminous=-0.4
```

A sky at 2048 texels a face takes about a minute on an integrated GPU; add
`--size 512` for a quick look.
