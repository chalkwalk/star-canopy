# Contributing to StarCanopy

StarCanopy is licensed **GPLv3**; contributions are accepted on those terms.

## Read these first

1. **`PRINCIPLES.md`** -- and the proposal gate at the top. If an idea cannot be
   expressed within the principles, say so in the issue; that is a useful
   conversation, not a rejection.
2. **`NON-GOALS.md`** -- if a proposal matches a fence, the answer is already
   written, and the reply is the alternative rather than "no".
3. **`AGENTS.md`** -- layout, commands, conventions, and the things most easily
   got wrong.

## Issues before pull requests

The issue forms ask for what each kind needs: **Bug report** for something that
fails, **This sky looks wrong** for a sky that renders but looks wrong -- its
project file makes it again, so include it -- and **Feature request**, which asks
which principle a proposal serves.

- **Changes to the look, new controls, architecture:** open an issue first.
  A change to the look is decided by a blind comparison (`PRINCIPLES §2`), so
  bring the renders, or expect to be asked for them.
- **Bug fixes, build fixes, documentation:** just open the pull request.

## Two rules that are not optional

- **No reference imagery.** Skies from games or photographs may inform numbers --
  brightness distributions, hue by lightness -- recorded with their provenance in
  `docs/references/SOURCES.md`. The images themselves never enter the repository
  (`fence #4`).
- **Licences are checked before code depends on them.** Third-party components
  must be GPLv3-compatible and are listed in `THIRDPARTY.md` in the same change
  that adds them (`PRINCIPLES §10`).

## Before opening a change

Work on a branch from `main`, and:

```bash
cmake --build build -j $(nproc)
ctest --test-dir build --output-on-failure
```

**Quote the output.** "Tests pass" is not evidence; the output is. The pull
request template carries the rest of the checklist.

## What CI checks

Every push and pull request to `main` builds on Linux, Windows and macOS.

- **Linux** runs the whole suite, the bake tests on Mesa's software OpenGL.
- **Windows** runs it on Mesa's software OpenGL too.
- **macOS** runs what the runner allows; the bake tests skip without an OpenGL
  3.3 context.

Windows and macOS are marked *unproven*: they build, but nothing has yet shown a
sky made on them to be right. If you can test on either, that is the help this
project most needs -- say so in your pull request.

## The manual

The manual is `website/docs/`, published to
[canopy.chalkwalkmusic.com](https://canopy.chalkwalkmusic.com) and mirrored to the
wiki. Edit it there, never on the wiki: the wiki is generated, and a sync
overwrites it.
