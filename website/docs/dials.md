---
sidebar_position: 5
---

# Raw dials

Under the macros are about eighty raw parameters: `starcanopy dials` lists each
with its default and range. They are for scripting, not steering -- the macros are
the controls -- and they are the base the macros move from: an override of
`exposure` is what `bright` doubles.

Set them in the project's `[overrides]`, or on the command line:

```bash
./build/starcanopy render night.toml --set galaxy-style=lenticular --set open-clusters=10
```

A few that are useful on their own:

| Dial | |
|---|---|
| `seed` | which sky; the same seed is always the same sky |
| `nebula` | `off` shows the galaxy and its stars alone |
| `galaxy-style` | the galaxy's type -- `auto` by seed, or `barred-spiral`, `grand-design`, `flocculent`, `lenticular` |
| `galaxy-radius`, `galaxy-height` | where the observer is, `auto` by seed |
| `open-clusters` | open clusters about a sky, typically; 0 none |
| `accent-near` | how strongly the sky's objects are drawn near |

A dial that belongs to a macro -- `galaxy-place`, which `galactic` moves -- is set
through its macro only.
