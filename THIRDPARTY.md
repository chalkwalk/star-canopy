# Third-party components

StarCanopy is GPLv3. Every component below is GPLv3-compatible. A component is
added to this list in the same change that makes the code depend on it.

**Status: planned.** Nothing is vendored yet; this is the intended set, to be
confirmed as each lands.

| Component | Use | Licence | Obligations |
|---|---|---|---|
| SDL3 | Window, input, GL context | zlib | Notice in documentation appreciated, not required |
| Dear ImGui | Interface | MIT | Keep the copyright notice |
| glad | OpenGL loader (generated) | MIT / public domain (generated code) | Keep the notice |
| `mtwist.c` (from Space Nerds In Space, Stephen M. Cameron) | Seeded random numbers, for seed compatibility with the labs | GPL-2.0-or-later | Keep its copyright header; usable under GPLv3 by "or later" |
| stb_image_write | PNG output | MIT or public domain | None beyond keeping the notice |
| tinyexr | OpenEXR output | BSD-3-Clause | Keep the notice |
| KTX-Software (libktx) | KTX2 output | Apache-2.0 | Keep NOTICE; Apache-2.0 is compatible with GPLv3 |
| A TOML parser (toml++) | Project files | MIT | Keep the notice |

The reference-sky measurements the palette space is fitted to are numbers, not
components; their provenance is recorded in `docs/references/SOURCES.md`.
