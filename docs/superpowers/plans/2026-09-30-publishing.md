# Publishing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** StarCanopy public on the user's GitHub with a history fit to publish, a website and synced wiki at `canopy.chalkwalkmusic.com`, and builds on Linux, Windows and macOS.

**Architecture:** First the tree and then the history are prepared locally (a backup branch kept), then the repository files the user's other projects have are added -- a three-platform `build.yml`, a Docusaurus site with a gallery, `deploy.yml`, `wiki-sync.yml` and its transform, issue and PR templates -- each checked locally, and last the repository is created empty and the history pushed, on the user's go-ahead.

**Tech Stack:** git and git-filter-repo; GitHub Actions; Docusaurus (Node 22, npm); Python 3 for scripts; the project's own CMake/ctest build.

**Spec:** `docs/superpowers/specs/2026-09-30-publishing-design.md`

## Global Constraints

- The reference game (whose skyboxes set the look's standard) is named once, in `README.md`, and nowhere else -- in code, parameters, docs, commit messages or history (`AGENTS.md` rule #4). Its name must not appear in this plan's commits either.
- The target game is named once, in `README.md`, and otherwise only in the licence credit the vendored generator requires (`THIRDPARTY.md`, `extern/mtwist/README.md`, its row in `docs/references/SOURCES.md`).
- No reference imagery anywhere in the tree or history (`PRINCIPLES §10`, fence #4). Gallery images are our own renders only.
- Stage files by name; never `git add -A`, `git add .` or `git commit -a`. Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Nothing is pushed, and no repository is created, without the user's explicit go-ahead at that moment** (Task 8). Force-pushing is never used.
- Untracked or excluded files stay out: `KICKSTART.md`, `blindweb/`, `.superpowers/`.
- Rewritten dates: each lift commit's author and committer dates are its last source commit's (the table in Task 2).
- Every task ends with `cmake --build build` clean and `ctest --test-dir build --output-on-failure` 100%, quoted.
- **The names live outside the tree.** Before Task 1, write the patterns to two files in `.git/info/` (never committed, never pushed): `.git/info/reference-game` -- one line per pattern: the reference game's name, its publisher's name, and the labs' lowercase star-field label in backticks with "star field" after it (read them from README.md line 148 and the fix commit "Name the reference game once"); `.git/info/target-game` -- the target game's name and its abbreviation as a word (from README.md line 15). Every check below reads them with `grep -i -f`. This plan names neither game, and nothing committed may.

## Review Focus

- The rewrite changing any file's final content: `main`'s tree after the rewrite must be byte-identical to the backup's (Task 2 checks `git diff backup main` is empty).
- The rewrite missing a naming of the reference game in some file version or message: the whole rewritten history is searched (Task 2's check).
- A workflow that is valid YAML but wrong for GitHub (a bad `on:` key, a missing permission): each workflow is linted with `actionlint` if available, and at least parsed, with its triggers and permissions checked against Antiphon's (Tasks 3, 4, 6).
- The site building with broken links or a missing CNAME: `npm run build` with `onBrokenLinks: 'throw'`, and the built `build/CNAME` checked (Task 4).
- The wiki transform producing links that do not resolve on the wiki: the transform is run locally on the docs and its links checked (Task 6).

---

### Task 1: The current tree -- names and provenance

**Files:**
- Modify: `AGENTS.md` (line 34-36, the paragraph on where the model lives; rule #4 gains the target-game line), `README.md` (line 15 and line 145 area), `DESIGN.md:300`, `ROADMAP.md:137` and `ROADMAP.md:377`, `docs/COMPLETED.md:36,59,66`, `docs/references/SOURCES.md:18`, `src/core/look.h:9`, `src/core/shaders/denoise.shader:84`, `test/test_noise.cpp:2`
- Keep: `THIRDPARTY.md:13`, `extern/mtwist/README.md`, `docs/references/SOURCES.md:71` (the licence credit)

- [ ] **Step 1: The check, which fails now**

```bash
{ git grep -n -i -f .git/info/target-game -- . ; git grep -n -i -e "the labs" -- . ; } | grep -v -e "^README.md:" -e "^THIRDPARTY.md:" -e "^extern/mtwist/README.md:" -e "^docs/superpowers/" -e "^docs/studies/" 
```
Expected: lines listed (AGENTS, DESIGN, ROADMAP, COMPLETED, SOURCES, look.h, denoise.shader, test_noise.cpp). The pass is done when this prints nothing but SOURCES' licence-credit row (and the README has exactly one mention).

(`docs/studies` and `docs/superpowers` record past work and speak of "the labs" as history; they name the target game nowhere -- check: `git grep -n -i -f .git/info/target-game -- docs/studies docs/superpowers` prints nothing.)

- [ ] **Step 2: The edits**
  - `AGENTS.md` 34-36: delete the paragraph "The model lives in ... `labs/features/nebula_sky/NEXT.md` there." In rule #4, after its last sentence, add: "The target game -- the one StarCanopy was first made for -- is likewise named once, in `README.md`, and otherwise only in the licence credit the vendored generator requires (`THIRDPARTY.md`, `extern/mtwist/`)." Remove "The labs' commit messages name it freely -- rewrite them in the lift." (done).
  - `README.md` 14-18 (the status note): "The sky model -- developed and judged blind over many rounds -- is here, ..." (no labs). At line 145's paragraph: one sentence -- "StarCanopy was first meant for [<the target game, linked as the README links it now>], and was made generic along the way; that game, which does not yet load HDR skyboxes, is still its first intended user. The whole is a work in progress." -- keeping the reference-game sentence as it is. This is the README's one naming of the target game.
  - `DESIGN.md:300`: "Game-specific layouts (a target game's face order and mirroring, for one)".
  - `ROADMAP.md:137`: delete the "Full notes: ..." sentence. `ROADMAP.md:377`: "- [ ] A target game that loads HDR skyboxes; a converter for its face order and mirroring".
  - `docs/COMPLETED.md` 36, 59, 66: "The model moved from its first home" / the base skybox named after the target game becomes "the old base skybox" / "which drops the old skybox". Read each sentence and reword it whole.
  - `docs/references/SOURCES.md:18`: "Everything below was measured or fitted during the model's development, from ..." (read the sentence; keep its meaning, drop the name).
  - `src/core/look.h:9`, `src/core/shaders/denoise.shader:84`: "the filmic curve the look was judged through" -- drop the attribution to a game; keep the rest.
  - `test/test_noise.cpp:2`: "Expected values were computed by the original implementation" (read and reword).

- [ ] **Step 3: Run the check and the build**

Run Step 1's command. Expected: only the SOURCES licence row. Run `git grep -n -i -f .git/info/target-game -- README.md`: exactly one line. Run `cmake --build build -j $(nproc) 2>&1 | grep -E "error|warning"; ctest --test-dir build 2>&1 | grep "tests passed"` -> `100% tests passed`.

- [ ] **Step 4: Commit**

```bash
git add AGENTS.md README.md DESIGN.md ROADMAP.md docs/COMPLETED.md docs/references/SOURCES.md src/core/look.h src/core/shaders/denoise.shader test/test_noise.cpp
git commit -F - <<'EOF'
Name the target game once, in the README

StarCanopy was first meant for one game and made generic; the README says so
once, and the licence credit the vendored generator requires keeps its name.
Elsewhere the model's first home drops out as provenance, the look is never
said to be judged through one game, and the game as a consumer of skies is "a
target game". AGENTS.md gains the rule.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 2: The history -- dates, the reference game's name, `main`

**Files:** none in the tree; history only. A script in the scratchpad, never committed.

- [ ] **Step 1: Back up**

```bash
git branch backup/pre-publish master
git log --oneline | wc -l   # note the count
```

- [ ] **Step 2: The check, which fails now**

```bash
git log --all --format=%B | grep -n -i -f .git/info/reference-game ; \
git rev-list master | while read h; do for f in PRINCIPLES.md NON-GOALS.md docs/references/SOURCES.md; do git show $h:$f 2>/dev/null | grep -q -i -f .git/info/reference-game && echo "$h $f"; done; done
```
Expected now: lines from the three messages (kick-start, "Record where the fitted numbers came from", "Drop per-file licence headers") and file versions before "Name the reference game once". The pass is done when this prints nothing (the README, allowed, is not searched).

- [ ] **Step 3: The rewrite script** (scratchpad `rewrite.py`, run with `git filter-repo --force --refs master --python-callback` style via its API):

```python
#!/usr/bin/env python3
"""Rewrite master: lift commits' dates from their last source commit, and the
reference game's name out of old messages and old versions of three docs."""
import subprocess, git_filter_repo as fr

# Lift commits (pre-rewrite short hashes) -> the last source commit's date.
DATES = {
    "8f874eb": "2026-09-23 13:02:54 -0700", "665e476": "2026-09-24 20:11:13 -0700",
    "fd679f3": "2026-09-23 13:02:54 -0700", "a139d49": "2026-09-26 10:35:36 -0700",
    "989c172": "2026-09-23 13:02:54 -0700", "ec9649e": "2026-09-24 20:11:13 -0700",
    "aaeb7b3": "2026-09-25 09:39:01 -0700", "de10c86": "2026-09-26 01:26:12 -0700",
    "0933545": "2026-09-23 13:02:54 -0700", "6a30ce5": "2026-09-25 23:59:27 -0700",
    "c3f5fa9": "2026-09-26 00:12:00 -0700", "d286102": "2026-09-26 09:45:52 -0700",
    "a33e3f3": "2026-09-26 11:42:59 -0700", "b1c5305": "2026-09-26 16:28:24 -0700",
    "78d0c58": "2026-09-26 19:57:24 -0700",
}
def epoch(iso):
    out = subprocess.run(["date", "-d", iso, "+%s %z"], capture_output=True, text=True).stdout.split()
    return f"{out[0]} {out[1]}".encode()

# The three messages, reworded whole where they name the reference game (and,
# in the same sentences, the target game): read each with `git log -1 --format=%B
# <hash>` and write its replacement here, keeping everything else verbatim.
MESSAGES = {
    "b8006cf": None,  # filled in Step 4 from the original, reworded
    "80ae3d6": None,
    "f2dd1b9": None,
}

# The old docs' text -> the fixed text, from the commit that fixed them:
# `git diff <fix>^ <fix> -- PRINCIPLES.md NON-GOALS.md docs/references/SOURCES.md`,
# each hunk's removed lines -> its added lines, as exact strings. Filled in Step 4.
REPLACE = []

full = {}
for short in list(DATES) + list(MESSAGES):
    full[subprocess.run(["git", "rev-parse", short], capture_output=True, text=True).stdout.strip().encode()] = short

def commit_cb(commit, metadata):
    short = full.get(commit.original_id)
    if short in DATES:
        commit.author_date = commit.committer_date = epoch(DATES[short])
    if short in MESSAGES and MESSAGES[short]:
        commit.message = MESSAGES[short].encode()

def blob_cb(blob, metadata):
    data = blob.data
    for old, new in REPLACE:
        data = data.replace(old.encode(), new.encode())
    blob.data = data

args = fr.FilteringOptions.parse_args(["--force", "--refs", "master"])
fr.RepoFilter(args, commit_callback=commit_cb, blob_callback=blob_cb).run()
```

- [ ] **Step 4: Fill the messages and replacements**, then show them to the user before running: for each of the three messages, the original and the reworded text side by side; for REPLACE, generate the pairs with a helper that parses the fix commit's diff hunks for the three files (removed block -> added block, exact), and print them. Each old string must be unique to those docs: check none occurs in any version of `README.md` (`git rev-list master | xargs -I{} git show {}:README.md | grep -c -F "<old>"` is 0).

- [ ] **Step 5: Run it, then verify**

```bash
python3 $SCRATCH/rewrite.py
git diff backup/pre-publish master --stat        # expected: empty -- the final tree unchanged
git log --oneline | wc -l                        # expected: the count from Step 1
```
Run Step 2's check on `master`: expected: nothing. Then `git log --format="%h %ad %s" --date=iso | grep Lift` shows the table's dates. Build and `ctest`: 100%.

- [ ] **Step 6: Rename, and keep the backup**

```bash
git branch -m master main
git branch   # expected: backup/pre-publish, * main
```
The backup stays until the user says it can go (after the push, in Task 9).

---

### Task 3: Builds on three platforms

**Files:**
- Create: `.github/workflows/build.yml`
- Delete: `.github/workflows/ci.yml`

- [ ] **Step 1: The check, which fails now**: `test -f .github/workflows/build.yml` fails.

- [ ] **Step 2: Write `build.yml`**, modelled on `/home/programming/antiphon/.github/workflows/build.yml` (read it first for the comments' style):

```yaml
# Build and test on Linux, Windows and macOS, and keep each one's starcanopy.
# The bake tests need OpenGL 3.3 and CI machines have no GPU: Linux uses Mesa's
# software renderer through EGL; Windows gets Mesa's software OpenGL installed
# beside the tests; on macOS they run if the runner gives a context and skip if
# not, as they do anywhere without a display.
name: Build

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  workflow_dispatch:

concurrency:
  group: build-${{ github.ref }}
  cancel-in-progress: true

jobs:
  build:
    name: ${{ matrix.name }}
    runs-on: ${{ matrix.os }}
    # Windows and macOS are unproven: this flag holds the door open while they
    # earn their place, and is taken off each as it does -- never used to make
    # a real breakage look green.
    continue-on-error: ${{ matrix.experimental }}
    strategy:
      fail-fast: false
      matrix:
        include:
          - name: Linux
            os: ubuntu-24.04
            experimental: false
          - name: Windows (build only, unproven)
            os: windows-latest
            experimental: true
          - name: macOS (build only, unproven)
            os: macos-latest
            experimental: true
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install dependencies (Linux)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends \
            ninja-build \
            libegl-dev libgl-dev libegl-mesa0 libgl1-mesa-dri \
            libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
            libxi-dev libxss-dev libxkbcommon-dev libwayland-dev \
            libdrm-dev libgbm-dev

      - name: Install Ninja (macOS)
        if: runner.os == 'macOS'
        run: brew install ninja

      - name: Configure (Linux and macOS)
        if: runner.os != 'Windows'
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

      - name: Configure (Windows)
        if: runner.os == 'Windows'
        run: cmake -B build

      - name: Build
        run: cmake --build build --config Release -j 4

      # Mesa's software OpenGL for Windows, beside the test executables, so the
      # bake tests have a 3.3 context on a runner with no GPU.
      - name: Install Mesa's software OpenGL (Windows)
        if: runner.os == 'Windows'
        shell: pwsh
        run: |
          $url = "https://github.com/pal1000/mesa-dist-win/releases/download/24.3.4/mesa3d-24.3.4-release-msvc.7z"
          Invoke-WebRequest $url -OutFile mesa.7z
          7z x mesa.7z -omesa | Out-Null
          Get-ChildItem build -Recurse -Filter "test_*.exe" | ForEach-Object {
            Copy-Item mesa/x64/*.dll $_.DirectoryName
          }

      - name: Test
        env:
          GALLIUM_DRIVER: llvmpipe
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Strip (Linux)
        if: runner.os == 'Linux'
        run: strip --strip-unneeded build/starcanopy

      - name: Upload starcanopy
        uses: actions/upload-artifact@v4
        with:
          name: starcanopy-${{ runner.os }}
          if-no-files-found: warn
          path: |
            build/starcanopy
            build/Release/starcanopy.exe
            build/starcanopy.exe
```

- [ ] **Step 3: Check it**: `python3 -c "import yaml,sys; d=yaml.safe_load(open('.github/workflows/build.yml')); assert d['jobs']['build']['strategy']['fail-fast'] is False; print(sorted(d[True].keys()) if True in d else sorted(d['on'].keys()))"` prints the triggers (PyYAML reads `on` as `True`); `actionlint .github/workflows/build.yml` if installed (`go install github.com/rhysd/actionlint/cmd/actionlint@latest` is not required; skip if absent and say so). Check the mesa-dist-win release URL resolves: `curl -sI <url> | head -1` shows 302 or 200; if not, pick the latest release's `-release-msvc.7z` from `https://api.github.com/repos/pal1000/mesa-dist-win/releases/latest`.

- [ ] **Step 4: Commit**

```bash
git rm .github/workflows/ci.yml
git add .github/workflows/build.yml
git commit -F - <<'EOF'
Build on Linux, Windows and macOS

In place of the Linux-only CI: a matrix of the three, Windows and macOS marked
unproven until they earn more; the bake tests on Mesa's software OpenGL on
Linux and Windows, and where the runner allows on macOS; each platform's
starcanopy kept as an artifact. Releases come later.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 4: The website -- the manual

**Files:**
- Create: `website/` -- `package.json`, `package-lock.json`, `docusaurus.config.ts`, `sidebars.ts`, `tsconfig.json`, `src/css/custom.css`, `src/pages/index.tsx`, `src/pages/index.module.css`, `src/components/HomepageFeatures/`, `static/CNAME`, `static/img/` (logo, favicon), `docs/*.md`
- Create: `.github/workflows/deploy.yml`
- Modify: `.gitignore` (add `website/node_modules/`, `website/build/`, `website/.docusaurus/`)

- [ ] **Step 1: The check, which fails now**: `cd website && npm run build` fails (no site).

- [ ] **Step 2: Scaffold from Antiphon**: copy `package.json`, `tsconfig.json`, `sidebars.ts`, `docusaurus.config.ts`, `src/` from `/home/programming/antiphon/website/`, then change: `name`/`title` to StarCanopy; `tagline` "Procedural space skyboxes, baked on the GPU"; `url: 'https://canopy.chalkwalkmusic.com'`, `baseUrl: '/'`; `organizationName: 'chalkwalk'`, `projectName: 'star-canopy'`; `editUrl` to `https://github.com/chalkwalk/star-canopy/tree/main/website/`; navbar and footer links to this repo; `onBrokenLinks: 'throw'` kept; the homepage features rewritten for StarCanopy (a sky from a seed; steered by macros; honest previews; HDR outputs for any engine). Remove any Antiphon-specific pages, images and components. `static/CNAME` = `canopy.chalkwalkmusic.com`. Then `npm install` to write this site's `package-lock.json`.

- [ ] **Step 3: The manual** in `website/docs/`, each page from the README's own text, kept in step with it (the README stays the canonical manual; the site presents it in pages): `intro.md` (what it makes, the status note, the target game sentence as in the README), `getting-started.md` (build, `new`, `render`), `projects.md` (project files, the command line, `--set` and `--macro`), `macros.md` (the thirteen macros, from `starcanopy macros` output and the README's macro section), `dials.md` (overrides, from `starcanopy dials`), `outputs.md` (OpenEXR, KTX2, PNG, the key-light sidecar, orientation, using them in an engine), `developers.md` (building, testing, AGENTS/PRINCIPLES/DESIGN/ROADMAP, pointing to CONTRIBUTING). Front matter `sidebar_position` in that order. No reference imagery; no reference game name.

- [ ] **Step 4: Build it**

```bash
cd website && npm ci && npm run build && cat build/CNAME
```
Expected: `[SUCCESS] Generated static files`; `canopy.chalkwalkmusic.com`. Any broken link fails the build (`onBrokenLinks: 'throw'`) -- fix the link, not the setting.

- [ ] **Step 5: `deploy.yml`**: copy `/home/programming/antiphon/.github/workflows/deploy.yml` as it is (its triggers `website/**` and itself, on `main`; Node 20 with npm cache on `website/package-lock.json`; `upload-pages-artifact` of `./website/build`; `deploy-pages`), keeping the same action versions as Antiphon's. Parse it with PyYAML as in Task 3.

- [ ] **Step 6: Commit**

```bash
git add website/package.json website/package-lock.json website/docusaurus.config.ts website/sidebars.ts website/tsconfig.json website/src website/static website/docs .github/workflows/deploy.yml .gitignore
git commit -F - <<'EOF'
Add the website: the manual, for canopy.chalkwalkmusic.com

A Docusaurus site, as the user's other projects have: the README's manual in
pages, built and published to GitHub Pages on changes to website/, a broken
link failing the build.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 5: The gallery

**Files:**
- Create: `website/docs/gallery.md`, `website/static/img/gallery/*.jpg`, `website/static/gallery/*.toml`
- Create (scratch, not committed): the render script

- [ ] **Step 1: The check, which fails now**: `test -f website/docs/gallery.md` fails.

- [ ] **Step 2: Choose and render**: twelve skies across seeds and macros -- the default look on several seeds; each family of macro at an end (open, dense, fragmented, billowing, luminous, vivid, galactic at -1 and 1, starry); a lenticular's sky; open clusters in view. For each: `./build/starcanopy new $S/g/NAME.toml --seed N`, set its `[macros]`, `./build/starcanopy render $S/g/NAME.toml --set size=2048`; show the user the equirect PNGs and let them veto any before they go on the site.

- [ ] **Step 3: Prepare for the web**: each `NAME_equirect.png` to a 2048x1024 JPEG at quality 85 (`python3 -c` with Pillow), about 0.5 MB each, into `website/static/img/gallery/`; each project file into `website/static/gallery/`.

- [ ] **Step 4: The page**: `gallery.md` -- for each sky, the image, a line on what it shows, and its project file as a code block and a download link; a note that any of them renders again exactly with `starcanopy render` (the look version recorded in each file).

- [ ] **Step 5: Build** as Task 4 Step 4: success, no broken links. Check the total size: `du -sh website/static/img/gallery` under 10 MB.

- [ ] **Step 6: Commit** by name (`website/docs/gallery.md website/static/img/gallery website/static/gallery`), message "Add a gallery of skies to the website", the why: every sky reproducible from the project beside it.

---

### Task 6: The wiki

**Files:**
- Create: `.github/workflows/wiki-sync.yml`, `.github/scripts/wiki_transform.py`

- [ ] **Step 1: The check, which fails now**: `test -f .github/scripts/wiki_transform.py` fails.

- [ ] **Step 2: Copy from Antiphon**: `/home/programming/antiphon/.github/workflows/wiki-sync.yml` and `/home/programming/antiphon/.github/scripts/wiki_transform.py` as they are; read the transform and change anything Antiphon-specific (page names, the site's URL -> `https://canopy.chalkwalkmusic.com`). The gallery's images are served from the site, so its image links must become absolute site URLs in the wiki -- check the transform does so, and add it if not.

- [ ] **Step 3: Run the transform locally**

```bash
rm -rf $SCRATCH/wiki && mkdir $SCRATCH/wiki && cp website/docs/*.md $SCRATCH/wiki/ && python3 .github/scripts/wiki_transform.py $SCRATCH/wiki
grep -n "](" $SCRATCH/wiki/*.md | grep -v "http" | head
```
Expected: the transform runs; every remaining relative link names a wiki page that exists in `$SCRATCH/wiki` (check each); images are absolute URLs.

- [ ] **Step 4: Commit** both files by name, message "Mirror the website's docs to the wiki", the why: one source for the manual, the wiki generated and never hand-edited.

---

### Task 7: Contributing, templates, README

**Files:**
- Create: `.github/ISSUE_TEMPLATE/bug_report.yml`, `.github/ISSUE_TEMPLATE/sky_looks_wrong.yml`, `.github/ISSUE_TEMPLATE/feature_request.yml`, `.github/ISSUE_TEMPLATE/config.yml`, `.github/PULL_REQUEST_TEMPLATE.md`
- Modify: `CONTRIBUTING.md`, `README.md` (badges; platforms line; site link), `ROADMAP.md` (tick the CI item after Task 8's first green run)

- [ ] **Step 1: The check, which fails now**: `ls .github/ISSUE_TEMPLATE` fails.

- [ ] **Step 2: Templates**, in the style of `/home/programming/antiphon/.github/ISSUE_TEMPLATE/*.yml` (issue forms):
  - `bug_report.yml`: what happened, what was expected, steps, the command and its output, platform and GPU, StarCanopy version (commit).
  - `sky_looks_wrong.yml`: title prefix "Sky: "; the project file (a textarea, `render: toml`) or seed and settings; the look version (from the project file); a screenshot at game field of view if possible; what looks wrong. The description: every sky renders again exactly from its project, so this is enough to see it.
  - `feature_request.yml`: the problem; the proposal; **which principle it serves** (a dropdown of PRINCIPLES' fourteen titles, `§1`..`§14`, read from PRINCIPLES.md); a checkbox "I have read NON-GOALS.md".
  - `config.yml`: `blank_issues_enabled: true`; a contact link to the website.
  - `PULL_REQUEST_TEMPLATE.md`: What this changes / Why / Checks: build clean and `ctest` green (output quoted); for a change to the look, the blind comparison and its score; for a new control, why it is not a macro; docs updated in the same commit; the reference game named nowhere but the README.
- [ ] **Step 3: `CONTRIBUTING.md`**: read it; bring it in line -- branch `main`; the three platforms and what CI checks on each; the templates; the site and wiki (docs are edited in `website/docs/`, never on the wiki).
- [ ] **Step 4: `README.md`**: badges for Build and the site under the title (`https://github.com/chalkwalk/star-canopy/actions/workflows/build.yml/badge.svg`, a link to `https://canopy.chalkwalkmusic.com`); a platforms line: "Tested on Linux. Builds on Windows and macOS, unproven."
- [ ] **Step 5: Check**: parse each `.yml` with PyYAML; the reference game's name nowhere (`git grep -n -i -f .git/info/reference-game -- . ':!README.md'` prints nothing); build and `ctest` 100%.
- [ ] **Step 6: Commit** by name, message "Add issue and pull-request templates, and bring CONTRIBUTING up to date".

---

### Task 8: The repository -- on the user's go-ahead

- [ ] **Step 1: Final checks**, quoted to the user: Task 2's check on `main` prints nothing; `git status --short` shows only `?? KICKSTART.md`; `git log --oneline | head -5`; the file count to be pushed (`git ls-files | wc -l`) and that none is over 10 MB (`git ls-files -z | xargs -0 du -k | sort -n | tail -3`).
- [ ] **Step 2: Ask** the user for the go-ahead to create the public repository and push `main`. Wait.
- [ ] **Step 3: Create empty, and push**

```bash
gh repo create chalkwalk/star-canopy --public \
  --description "Procedural space skyboxes, baked on the GPU in HDR: a seed for the composition, macros to steer it" \
  --homepage https://canopy.chalkwalkmusic.com
gh repo edit chalkwalk/star-canopy --add-topic procedural-generation,skybox,hdr,opengl,gamedev,space
git remote add origin https://github.com/chalkwalk/star-canopy.git
git push -u origin main
```
(`gh repo create` without `--add-readme`, `--license` or `--gitignore` makes it empty.)
- [ ] **Step 4: Watch the first runs**: `gh run list --limit 5`; `gh run watch` on the Build run. Linux must go green; if it fails, fix on `main` with the usual commit and push (never force). Windows and macOS: record their state -- green, or what failed -- in the final report; they are marked unproven either way.

---

### Task 9: What only the user can do, and the last checks

- [ ] **Step 1: Tell the user**, with exact values:
  1. DNS for `chalkwalkmusic.com`: CNAME `canopy` -> `chalkwalk.github.io`.
  2. `https://github.com/chalkwalk/star-canopy/settings/pages`: Source "GitHub Actions"; Custom domain `canopy.chalkwalkmusic.com`; "Enforce HTTPS" once the certificate is issued.
  3. `https://github.com/chalkwalk/star-canopy/settings`: Features -> Wikis on; then the Wiki tab, "Create the first page", save anything.
- [ ] **Step 2: When they report done**: re-run Deploy (`gh workflow run deploy.yml`) and Wiki sync (`gh workflow run wiki-sync.yml`); check `curl -sI https://canopy.chalkwalkmusic.com | head -1` is 200 (DNS may take time: say so rather than retrying in a loop); the wiki lists the docs' pages.
- [ ] **Step 3: Tick** ROADMAP's "CI on Linux" item (and record Windows/macOS state), commit and push. Ask whether `backup/pre-publish` can be deleted; delete it only on a yes.
