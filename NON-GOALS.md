# StarCanopy -- Non-Goals

> This file is the long form of the *Non-Goals* index in `PRINCIPLES.md`. It
> exists because a focused tool is defined as much by what it refuses as by what
> it does. Each fence below names what **prompted** it, the **principle that
> rejects it**, and -- crucially -- **what we offer instead**. A non-goal is not a
> gap; it is a decision.
>
> Read this alongside `PRINCIPLES.md`. When a feature request arrives, check here
> first: if it matches a fence, the answer is already written, and the reply is
> the alternative, not "no".
>
> Fences are cited as `fence #N` and the numbers are stable anchors. The index
> table in `PRINCIPLES.md` mirrors this numbering exactly. More fences will be
> added as we meet their edges; a new fence is a deliberate change to both files.

## The three fences

Almost every refusal traces to one of three costs:

- **It lays the look on top instead of growing it.** StarCanopy's contrast comes
  from cloud with form, lit (`PRINCIPLES §4`). Anything that paints, filters or
  imitates a finished image skips the part that makes the sky read as a place.
- **It makes the tool about its parameters, not its sky.** A control a user cannot
  understand, or a mode they cannot come back from, trades a good sky for a
  configurable one (`PRINCIPLES §5`).
- **It is the game's job, not the sky's.** StarCanopy bakes a texture. What moves,
  what orbits, what is lit in the foreground and what runs every frame belongs to
  the game and its engine, which do those things better.

The test is constructive: we reject the *form*, then offer the version of the same
desire that fits.

## The catalogue

| # | Non-goal | Prompted by | Rejected by | What we offer instead |
|---|---|---|---|---|
| 1 | **Brush-stroke or painterly stylisation filters** | Some well-known game skyboxes show one; a Kuwahara filter was prototyped here and looked attractive | §4, §2 | *Constructed* detail: large masses blocked in, detail sculpted into their surfaces where form needs it. "Painterly" in this project means built like a painting, not textured like one. |
| 2 | **Runtime or in-engine rendering** -- an engine plugin, a shader to ship, a bake at level load | The bake is GPU code and could run anywhere a game runs | §7, §8 | Baked textures in standard formats, HDR first, which every engine already loads. A game that wants variety ships several, or bakes offline in its own pipeline with the command line. |
| 3 | **AI image generation or learned models** | The obvious modern route to "make me a nebula" | §10, §3 | A procedural model whose every term is documented, seeded and reproducible, and whose provenance can be stated. |
| 4 | **Presets that imitate a named product; reference imagery in the tree** | An admired game's skyboxes are this project's standard | §10 | A palette space and tone targets fitted from *measurements* of reference skies, recorded with their provenance in `docs/references/SOURCES.md`. Every sky is of the references' kind without copying one. |
| 5 | **Animated or time-varying skies** | Nebulae that drift, stars that twinkle | North Star | A static sky, which is what a skybox is. Rotation over time, if a game wants it, is one line in the engine. |
| 6 | **A CPU rendering path** | Portability to machines without a capable GPU | §9 | GPU rendering on OpenGL 3.3-class hardware on Linux, Windows and macOS. At these step counts a CPU takes minutes to hours per sky, which would break the preview ladder that the whole workflow rests on. |
| 7 | **Every parameter as a control; an "advanced mode"** | The model has about a hundred parameters, and exposing them is easy | §5 | Macros with descriptive names, additive so they compose. Raw parameters as **command-line overrides**, for scripting, where nobody strays off the macros by accident. |
| 8 | **Suns, planets, ships and foreground objects** | "A sky needs a sun" | North Star, §8 | An HDR sky a game can project its own sun into, and the key light's direction reported with the files so the scene's light can match (`PRINCIPLES §14`). |

## Notes on the close calls

**No advanced mode is not no access (#7).** The raw parameters exist and are
reachable -- by command-line override, in a project file, in a script. What is
refused is a panel of them in the interface, because every such panel becomes the
place people go, and from there the macros no longer describe their sky and there
is no road back. If a raw parameter turns out to be something users keep reaching
for, the answer is a macro, or a change to one.

**Physics is not a fence either way (#3, and §3).** Refusing learned models is not
a claim that everything here is physical: some terms are not, and are kept because
they won blind. The fence is about provenance and reproducibility, not realism.

**Bakes in someone else's pipeline are fine (#2).** A studio running the command
line in its build to make a sky per level is using StarCanopy as intended. What is
refused is StarCanopy becoming the thing that runs inside the game.

---

When in doubt, the question is never "can the model do it?" -- it is "would a game
developer looking around this sky, at their camera's field of view, think it
better, and could they have got there without a manual?"
