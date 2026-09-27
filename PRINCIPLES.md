# StarCanopy -- Principles

> The principles below are the touchstones for every decision in this project.
> When a proposal conflicts with one of them, the proposal is the thing that has
> to bend.
>
> They are deliberately opinionated. StarCanopy is not a neutral renderer; it is a
> stance about what a space game's sky is for and how you arrive at a good one.
> The principles exist so that stance survives contact with feature requests,
> contributors, and our own future temptations.
>
> Read this file before adding a work area to `ROADMAP.md`, before opening a
> section in `DESIGN.md`, and before approving a feature idea. If an idea cannot
> be expressed within these principles, it does not belong in StarCanopy -- or the
> principles need revising first, deliberately, not silently.
>
> **Citing principles.** Principles are referenced by number (`PRINCIPLES §N`)
> across `DESIGN.md`, `ROADMAP.md`, `NON-GOALS.md` and source comments, and the
> `NON-GOALS.md` fences are referenced as `fence #N`. The numbers are **stable
> anchors** -- do not renumber casually. The short titles are mnemonics, not the
> citation key. If a renumber is ever unavoidable, sweep every `PRINCIPLES §N`
> and `fence #N` reference in the docs **and** in `src/` in the same change.

## North Star

StarCanopy makes the sky of a place in space: a nebula and a galaxy of stars
around the player, baked into a cubemap a game loads. The sky should read as
physical and deep -- cloud with form, lit by something, with open sky between
where the stars show -- and it should look good **where a player will see it**,
through a game camera, not only as a map of the whole sphere.

You get there by starting from a seed and steering with controls that describe
the sky in words, looking around as you go. What you take away is the texture.

The name is the design. A *canopy* is the dome overhead that a place is under.
StarCanopy makes that dome, and only that dome: not the sun, the planets or the
ships under it (`fence #8`).

Where two principles are in tension, the North Star is the tie-breaker: the
reading that gives a game developer a better-looking sky, sooner, with less to
understand, wins.

## How to use this document

Run a proposal through this gate **before** it reaches `DESIGN.md`. Each rung
names what the proposal must answer and the principle that owns the test. If any
answer is "no" or "well, except...", the proposal bends, not the principle.

1. **Looking** -- How will it be judged, and is that at game field of view, in a
   look-around? (§1)
2. **Evidence** -- If it changes the look, what blind comparison says it is
   better? If it quotes a number, does that number track the blind scores? (§2)
3. **Physics** -- Is it physical because that earned the look, or because it is
   physical? (§3)
4. **Form** -- Does the contrast come from form and light, or is it laid on top?
   (§4)
5. **Controls** -- Would a user understand the control it adds, and does it keep
   them on the macros rather than stranding them off them? (§5)
6. **Seed** -- Is the result still a function of seed and controls alone? (§6,
   §7)
7. **Output** -- Does it survive into the HDR texture, and is the 8-bit version
   derived from that? (§8)
8. **Preview** -- Does the preview run the same pipeline as the final render? (§9)
9. **Provenance** -- Is everything it uses first-party or compatibly licensed, and
   is any reference used only as numbers, with that recorded? (§10)
10. **Open sky** -- Does it leave room for the stars? (§11)
11. **Reader** -- Is every control it adds named, described, and reachable from
    the keyboard and the command line? (§12)
12. **Duplication** -- Is any rule it needs already defined somewhere? (§13)
13. **Orientation** -- Can a game developer still point the sky where their scene
    needs it? (§14)
14. **Fences** -- Does it clear the `NON-GOALS.md` catalogue?

---

## 1. A sky is judged where it is seen

A skybox is not looked at as a map. A player sees perhaps a sixth of it at a time,
through a game camera of 45 to 90 degrees, often while moving. A sky that reads
well in a whole-sphere projection can read as soft and indecisive at that field of
view: the first sky tried in a game had exactly that problem, with cloud edges a
degree and a half wide -- forty screen pixels of fade.

**Consequence.** The primary way to judge a sky, for us and for a user, is to
**look around in it** at a game-like field of view. Whole-sky projections (Equal
Earth, for its low distortion) are for composition and for comparing many skies
at once; they never settle a question about edges, detail or grain.

**Consequence.** Looking around has to be cheap and easy, because it is the
instrument: a look-around at every stage of the preview ladder (§9), side-by-side
and A/B views of two candidates, and collections of options to compare. Tooling
that makes looking easier is core work, not polish.

## 2. Judged by eye, blind; measured to explain

StarCanopy has no correctness bar to compute against. It has an aesthetic
standard: a small set of admired skyboxes from a published space game, measured
against not as skies to reproduce but as the level of look to reach. The arbiter
is aesthetic judgement, made carefully.

**Consequence.** A change to the look is decided by a **blind comparison**:
candidates side by side, each variant on the left in half the pairs, shuffled, the
key opened only after scoring. A change that loses, or that nobody can tell apart,
does not ship as a default, however principled it is.

**Consequence.** Metrics are instruments for *explaining* a look, and are kept
only while they track the blind scores. They have pointed the wrong way: a
candidate that matched the reference skies' tone distribution far better than the
default lost to it 10-0, because it hid the open sky (§11). A measure that has not
been checked against scores is a hypothesis, and says so where it is quoted.

**Consequence.** Measure what is there before explaining it. More than once a
supposed defect was noise, and a supposed cause was not the cause; a pixelwise
difference or a toggle-each-suspect table comes before a theory.

## 3. Physics when it earns the look

What matters is what the sky looks like. Where physics gives that look, physics
is preferred, because a mechanism keeps giving -- an ionisation front puts a
bright rim on every lit fold without anyone placing one, and pillars point at the
stars that carved them. But physics is a means.

**Consequence.** A physical model competes blind like anything else (§2).
Physically placed dust, casting shadows and lit like the gas, lost 0-9 to no dust
at all; it is off. A non-physical term -- a dim light raking across faces the
stars miss -- is in, because it won.

**Consequence.** When a non-physical term is used, it is named for what it does
and documented as a choice, so nobody later "corrects" it toward physics and
loses the look.

## 4. Form and light, not decals

A sky's contrast should come from cloud having shape and being lit: bright rims,
faces turned from the light, silhouettes against glow, open sky between. The
opposite is contrast laid on top: a dark lane painted by a noise and a rule, a
stylising filter, a texture that does not belong to the lighting.

**Consequence.** Every layer that darkens or brightens must belong to the light
transport -- it absorbs, emits or scatters, and it shadows and is shadowed. Dust
drawn as its own ramp, regardless of the light, read as "pasted on" and was
removed. A brush-stroke filter was tried and refused (`fence #1`).

**Consequence.** Detail goes where form needs it: large masses blocked in first,
then detail sculpted into their surfaces -- constructed, the way a painting is,
rather than equal detail sprinkled everywhere.

## 5. Controls a user can understand

The model underneath has on the order of a hundred parameters. Each is defensible
on its own, many interact, several only make sense in a narrow range, and the sum
is a surface nobody can form an opinion about.

**Consequence.** The surface is **macros**: a small set of controls with names that
describe the sky ("open", "billowing", "luminous"...), each a function of many
parameters, combined **additively** so that one parameter can serve several
macros. A macro earns its place by being understood without a manual and by
moving the sky in the direction its name says.

**Consequence.** There is no "advanced mode" of raw parameters in the interface.
An advanced mode is a place you cannot come back from: once you have moved a raw
parameter, the macros no longer describe the sky. Raw parameters are reachable as
**command-line overrides** only, for the person who knows exactly what they are
doing and is scripting it (`fence #7`).

**Consequence.** Expose what a user can understand, not as much as possible. A
proposal that adds a control must first fail to be expressed as a macro, or as a
change to an existing one.

## 6. The seed is the starting point; the controls are the power

A seed gives a composition: where the masses are, where the light is, which way
the galaxy runs. That is the right place to start, and browsing seeds -- a grid of
small skies, pick one -- is the fastest way to find a starting point.

**Consequence.** Browsing seeds is blessed, with its eyes open: it is
deterministic and revisitable, the same seed is always the same sky. But the
power is in the hands-on control after it. If browsing seeds ever becomes the
main way people get a sky they like, that is the signal to improve the macros,
not to add more seeds.

## 7. The texture is the product, and it can be made again

What a user takes away is a texture. A project file -- version, seed, macros,
overrides -- is how that texture is made again, and is cheap to keep and share.

**Consequence.** The same version, on the same machine, renders a project to the
same pixels. On another GPU it renders the same sky -- different drivers do
floating-point arithmetic differently, so the promise is the sky, not the bits.

**Consequence.** The look is **versioned**. Improving the model must not silently
change an existing project's sky; a project records the look version it was made
with, and a newer version either renders that look or says plainly that it cannot.

**Consequence.** Nothing in a render depends on anything but the project: no time,
no machine identity, no unseeded randomness.

## 8. HDR first

A sky is light, and games increasingly light their scenes from it. The primary
output is **linear HDR** radiance; an 8-bit, tonemapped image is derived from it,
never the other way round.

**Consequence.** Nothing is clipped in the model. The bright heart of a nebula
keeps its value, so a game can take reflections and ambient light from the sky,
and can project its own sun into it (`fence #8`).

## 9. Previews are honest

Speed comes from resolution, never from a different algorithm. The preview
ladder is one pipeline at four sizes: a grid of **128** per face to choose from, a
**256** look-around to steer, **1024** to approve, and the export at **2048** or
whatever is chosen.

**Consequence.** What you approve is what you get. A shortcut that would make a
preview disagree with the final render -- a cheaper light model, a different
noise, a skipped term -- is refused, because the user would be steering by a sky
they will not receive.

**Consequence.** Resolution-dependent behaviour is designed in: detail is
band-limited to what a texel can hold, so a small preview is the same sky seen
more coarsely, not a different one.

## 10. First-party or compatibly licensed, with provenance recorded

StarCanopy is GPLv3. It uses well-maintained third-party components under
compatible licences where they exist, and writes its own where they do not.

**Consequence.** `THIRDPARTY.md` lists every component and its obligations.
`docs/references/SOURCES.md` records where each model and each fitted constant
came from.

**Consequence.** Reference skies are used as **numbers only**: brightness
distributions, hue and chroma by lightness. No reference imagery enters this
repository, at any point in its history, and no preset imitates a named product
(`fence #4`).

## 11. Open sky is part of the sky

The star field and the galaxy are not what shows through when there is no nebula;
they are half of the look. A sky that fills every direction with cloud is
imposing, and will not suit every game.

**Consequence.** The stars and the galaxy are first-class, modelled with the same
care as the gas, and every look keeps room for them. Sparse skies -- one region, a
few clouds, a band -- are as much in scope as enveloping ones.

## 12. Accessible where it can be, honest where it cannot

Judging a sky needs sight; a tool whose product is an image cannot be made
equally usable without it, and we say so rather than claim otherwise.

**Consequence.** Everything that can be served is served: every control is named,
described and reachable from the keyboard; macros are described in words; and the
command line is a full equal of the interface, so anything the interface can make,
a script can make.

## 13. One definition, shared

A rule with more than one consumer gets exactly one home.

**Consequence.** The galaxy's density has a CPU twin, for placing stars, and a GPU
twin, for its glow; they are one specification and are tested against each other.
The macro table is the one definition of what a macro does, read by the
interface, the command line and the documentation alike. A second copy of either
is how the two drift apart.

## 14. The game decides where the sky points

A sky has a key light and a composition, and a game developer needs both to agree
with their scene -- most of all, the sky's brightest light with the scene's sun.

**Consequence.** Orientation is an output decision, not a regeneration: a sky can
be rotated and reprojected on export, with a compass and axes in the look-around
to orient by, and the direction of its key light is reported with the files, so a
scene's directional light can be set to match.

---

## Non-Goals -- what StarCanopy refuses to become

The full catalogue, with what prompted each refusal and what we offer instead, is
in `NON-GOALS.md`. This index is a pointer, not a second copy -- edit the fence
text there.

| # | Non-goal | Rejected by |
|---|---|---|
| 1 | Brush-stroke or painterly stylisation filters | §4, §2 |
| 2 | Runtime or in-engine rendering | §7, §8 |
| 3 | AI image generation or learned models | §10, §3 |
| 4 | Presets that imitate a named product; reference imagery in the tree | §10 |
| 5 | Animated or time-varying skies | North Star |
| 6 | A CPU rendering path | §9 |
| 7 | Every parameter as a control; an advanced mode | §5 |
| 8 | Suns, planets, ships and foreground objects | North Star, §8 |
