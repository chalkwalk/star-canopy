# Publishing: design

*2026-09-30. Agreed in a Q&A with the user. Modelled on the user's projects
Antiphon and Arps Euclidya (`chalkwalk/antiphon`, `chalkwalk/arps-euclidya`).*

## What it is for

StarCanopy goes public: a repository on the user's GitHub, a website with the
manual and a gallery of skies, a wiki kept in step with it, and builds on
Linux, Windows and macOS -- as the user's other projects have. Success: the
repository public at `github.com/chalkwalk/star-canopy` with a history fit to
publish; the site live at `canopy.chalkwalkmusic.com`; the wiki mirroring the
site's docs; every push building on three platforms and testing where it can;
a newcomer able to build, render a sky and contribute from what they find
there.

## 1. The history, before any push

A push cannot be taken back -- it may be cached or indexed at once -- so the
history is made fit to publish first, locally, with a backup branch of the old
history kept until the user is satisfied.

- **Date the lift commits** (agreed 2026-09-26, the date settled 2026-09-30):
  each commit that lifted finished work from the model's original development
  takes the date of the **last** commit it draws on there. The mapping of lift
  commits to their source commits is in the plan.
- **Scrub the reference game's name** -- the game whose skyboxes set the look's
  standard, which AGENTS.md's rule #4 allows only once, in the README -- from
  the three commit messages that carry it (the kick-start, the record of where
  the fitted numbers came from, and the dropping of licence headers, which
  names the labs' star-field mode after it), and from PRINCIPLES.md,
  NON-GOALS.md and SOURCES.md as they stood before it was removed from them.
  Sentences in those messages that name the target game alongside it are
  reworded whole. The README's one naming stays, as the rule allows.
  *(Corrected 2026-09-30: this said "the target game"; the rule, and this pass,
  are about the reference game.)*
- **The current tree**, as its own commit before the rewrite:
  - the model's development "in the labs" is dropped as provenance throughout
    (AGENTS, COMPLETED, SOURCES, ROADMAP, a test comment) -- it matters to no
    one reading the project now;
  - the look is never said to be judged through, or fitted to, one game
    (`look.h`, `denoise.shader`);
  - the game as a consumer of the skies becomes "a target game" (DESIGN,
    ROADMAP);
  - the licence credit the vendored generator requires stays (`THIRDPARTY.md`,
    `extern/mtwist/README.md`, its row in SOURCES);
  - the README says once that StarCanopy was first meant for that game, was
    made generic, and that the game -- which does not yet load HDR skyboxes --
    is the first intended user; the whole a work in progress;
  - AGENTS.md gains a line beside rule #4, which stands as it is for the
    reference game: the target game, too, is named once, in README.md, and
    otherwise only in the licence credit the vendored code requires.
- **Rename `master` to `main`**, GitHub's default for a new repository and the
  user's other projects' branch.

## 2. The repository

`chalkwalk/star-canopy`, public -- GitHub Pages on a free plan needs it -- and
created **empty**, with no starter README, licence or `.gitignore`, so the
prepared history goes in as it is with nothing to rebase onto. A description
and topics (procedural, skybox, HDR, OpenGL, game development). The push is
outward-facing: the user gives the final go-ahead at that moment.

Untracked and excluded files stay out: `KICKSTART.md`, `blindweb/`
(`.git/info/exclude`), `.superpowers/` (ignored).

## 3. Builds: `.github/workflows/build.yml`

In place of the Linux-only `ci.yml`, as Antiphon's `build.yml`:

- A matrix of Linux, Windows and macOS, `fail-fast: false`; Windows and macOS
  `continue-on-error` and named "(build only, unproven)" until they earn more,
  the flag never used to make a real breakage look green.
- Submodules checked out recursively; Ninja on Linux and macOS; the Visual
  Studio generator on Windows.
- Tests:
  - Linux: the full suite, the bake tests on Mesa's software OpenGL through
    EGL, as `ci.yml` did;
  - Windows: Mesa's software OpenGL installed in the job, so the bake tests
    have a context;
  - macOS: what the runner allows -- the bake tests skip where there is no
    OpenGL 3.3 context, as they do without a display.
- Each platform's `starcanopy` binary uploaded as a workflow artifact
  (stripped on Linux). **Releases later**: tagged GitHub Releases come once
  there is something to release -- the interface, or a first stable command
  line.
- On pushes and pull requests to `main`, and by hand.

## 4. The website and the wiki

- **Docusaurus** under `website/`, as Antiphon's: `url`
  `https://canopy.chalkwalkmusic.com`, `baseUrl` `/`, `onBrokenLinks: 'throw'`,
  `website/static/CNAME` holding `canopy.chalkwalkmusic.com`.
- **The manual**, from the README, in `website/docs/`: what StarCanopy is;
  getting started (build, a first sky); projects and the command line; the
  macros; raw dials; outputs and using them in an engine; developers (pointing
  to CONTRIBUTING).
- **A gallery**: a dozen skies rendered at export size across seeds and macros,
  each with the project settings that make it again. Our own renders only --
  never reference imagery (`PRINCIPLES §10`, fence #4).
- `.github/workflows/deploy.yml`: build the site and publish it to Pages on
  changes to `website/`, as Antiphon's.
- `.github/workflows/wiki-sync.yml` and `.github/scripts/wiki_transform.py`:
  the docs mirrored to the wiki on every change, generated and never
  hand-edited, as Antiphon's; a wiki not yet initialised reported in the run's
  summary, not failed.

## 5. Contributing

- `.github/ISSUE_TEMPLATE/`:
  - a bug report;
  - **"This sky looks wrong"**, asking for the project file (or seed and
    settings), the look version and a screenshot -- every sky is reproducible
    from its project, so every such report can be rendered again exactly;
  - a feature request that asks which principle it serves (the proposal gate
    at the top of PRINCIPLES.md).
- `.github/PULL_REQUEST_TEMPLATE.md`: the checklist from AGENTS.md's *Before
  claiming a change works* -- build clean and tests green, quoted; a blind
  comparison for a change to the look; a control justified as not a macro;
  docs shipped with the code.
- `CONTRIBUTING.md` reviewed for the new workflow and platforms.
- The README: build and site badges, and an honest line on platforms -- tested
  on Linux, building on Windows and macOS but unproven.

## What only the user can do

1. At the DNS provider for `chalkwalkmusic.com`: a CNAME record `canopy` ->
   `chalkwalk.github.io`.
2. In the repository's settings: Pages to deploy from GitHub Actions, and the
   custom domain `canopy.chalkwalkmusic.com` (with HTTPS enforced once the
   certificate is issued).
3. Enable the wiki and save one placeholder page, so the sync has somewhere to
   write.

## Out of scope

Tagged releases; code signing or notarisation; package-manager listings; a
blind-round or scoring tool on the site (`blindweb` stays local). Globular
clusters, paused at their design's section 4, resume after this.
