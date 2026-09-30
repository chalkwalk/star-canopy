---
sidebar_position: 3
---

# Projects and the command line

A **project** is how a sky is made again: a small TOML file with the look version
it was made with, a seed, a style (`mass`, for now the only one), macros,
overrides, orientation and outputs. `starcanopy new` writes one with every key
commented.

The same project renders the same sky, on any machine. A project records the
**look version** it was made with; a project from a newer StarCanopy is refused
rather than rendered differently, and one from an older look says how to render it
with the current one.

## Commands

```text
starcanopy new PROJECT.toml [--seed N]
starcanopy render PROJECT.toml [--macro NAME=VALUE]... [--set NAME=VALUE]...
                  [--size N] [--out DIR] [--context K]
starcanopy macros
starcanopy dials
```

| Option | |
|---|---|
| `--macro NAME=VALUE` | a macro, -1 to 1, over the project's own; repeatable |
| `--set NAME=VALUE` | a raw dial's base, over the project's own; repeatable, for scripting |
| `--size N` | texels per face, over the project's |
| `--out DIR` | where to write, over the project's |
| `--context K` | `auto` (EGL, else a hidden window), `egl` (headless only) or `window` |

`starcanopy macros` lists the macros and the dials each one moves;
`starcanopy dials` lists every raw dial with its default and range.
