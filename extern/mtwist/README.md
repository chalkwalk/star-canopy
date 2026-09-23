# mtwist -- the Mersenne Twister from Space Nerds In Space (vendored)

`mtwist.c` and `mtwist.h` are copied unchanged from
[Space Nerds In Space](https://github.com/smcameron/space-nerds-in-space), where
Stephen M. Cameron added them in commit `e8ad8165` (2014). GPL-2.0-or-later, which
StarCanopy takes under GPLv3 by its "or later" clause; the files keep their own
header.

Kept, rather than replaced by `std::mt19937`, so that a seed makes the same sky
here as it did in the SNIS labs where the model was developed: every scene,
galaxy and star field is drawn from it. Do not edit these files; a change to a
single number drawn is a change to every sky.
