# tinyexr -- OpenEXR reading and writing (vendored)

From [syoyo/tinyexr](https://github.com/syoyo/tinyexr) at tag `v3.2.0`, unchanged:
`tinyexr.h`, the two headers it includes (`exr_reader.hh`, `streamreader.hh`),
and the `miniz.c`/`miniz.h` it ships in `deps/miniz` for ZIP compression.
`impl.cpp` is ours: the one translation unit that compiles the implementation.

Licences: tinyexr BSD-3-Clause (`LICENSE`), including code from OpenEXR under
ILM's BSD-style licence (in the header of `tinyexr.h`); miniz MIT
(`LICENSE.miniz`). The parts of tinyexr's NOTICE that name other licences
(fpnge, Apache-2.0) cover files under its `src/`, which are not vendored.
