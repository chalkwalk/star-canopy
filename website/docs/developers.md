---
sidebar_position: 8
---

# Developers

StarCanopy is C++17 with CMake, OpenGL 3.3 and GLSL 1.50, SDL3 and Dear ImGui.
The model and the bake live in `src/core/`, the command line in `src/cli/`, the
study tools in `tools/`, the tests in `test/`.

```bash
cmake -B build
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure
```

The bake tests need an OpenGL 3.3 context: on Linux, EGL headless -- Mesa's
software renderer will do, slowly.

Before a change, read the project's documents, in this order:

- [PRINCIPLES.md](https://github.com/chalkwalk/star-canopy/blob/main/PRINCIPLES.md) --
  why, and the gate every proposal passes;
- [NON-GOALS.md](https://github.com/chalkwalk/star-canopy/blob/main/NON-GOALS.md) --
  what it refuses, and what it offers instead;
- [DESIGN.md](https://github.com/chalkwalk/star-canopy/blob/main/DESIGN.md) -- the
  model and the architecture;
- [ROADMAP.md](https://github.com/chalkwalk/star-canopy/blob/main/ROADMAP.md) --
  what is next;
- [AGENTS.md](https://github.com/chalkwalk/star-canopy/blob/main/AGENTS.md) -- how
  to work in the repository.

Contributions are welcome: see
[CONTRIBUTING.md](https://github.com/chalkwalk/star-canopy/blob/main/CONTRIBUTING.md).
A change to the look ships only after a blind comparison, scored by eye.

StarCanopy is free software under the GPLv3.
