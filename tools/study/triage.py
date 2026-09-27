#!/usr/bin/env python3
"""The triage table: every dial's class, with the study's numbers beside it.

  triage.py OAT_MASS OAT_SHELL FINE_MASS FINE_SHELL [MORRIS_MASS MORRIS_SHELL]

The class is a decision, written here with its reason; the numbers are what it
was decided from. A class the numbers contradict is flagged, so a rerun of the
study cannot silently disagree with the triage.
"""
import csv
import statistics
import sys
from collections import defaultdict

# The decisions. Anything not named is "look".
QUALITY = {"step-frac", "max-steps", "light-res", "light-steps", "supersample", "denoise", "galaxy-res"}
DEBUG = {"nebula": "the gas off, to judge the stars and galaxy alone",
         "grade": "physical: the lines' own colours, ungraded, to judge the physics",
         "line-colors": "the lines' colour mappings; under the grade they only reweight brightness"}
DECIDED = {"spike": "diffraction spikes, off: baked into a sky they read as a telescope's artefact",
           "spike-flux": "which stars get spikes; nothing while spike is off",
           "mass-dust": "dust in the mass: every kind lost to none, blind, three times",
           "mass-edge": "hard edges: pending their blind A/B (ROADMAP, Hard edges)",
           "mass-edge-patch": "where the edges are hard; nothing while mass-edge is off"}
SHELL = {"clusters", "cavity-density", "cavity-spread", "blister", "cluster-size", "dust", "dust-style",
         "dust-scale", "dust-opacity", "pillars", "pillar-length", "pillar-width", "pillar-density",
         "clouds", "cloud-length", "cloud-width", "cloud-distance", "cloud-density", "grade-dust",
         "rim-shadow", "outer-sharpness", "thickness", "distant-count", "distant-min-deg",
         "distant-max-deg"}
SHELL_NOTE = {
    "clusters": "the mass's twin is mass-clusters",
    "cavity-density": "the mass's twin is mass-cavity", "cavity-spread": "with cavity-density",
    "blister": "the mass's twin is mass-blister", "cluster-size": "the mass's twin is mass-cluster-size",
    "dust": "the mass's twin is mass-dust (off)", "dust-style": "with dust",
    "dust-scale": "with dust", "dust-opacity": "with dust", "grade-dust": "with dust",
    "rim-shadow": "the mass's grazing shadows take its place",
    "outer-sharpness": "the shell's outer edge, and the distant nebulae's",
    "thickness": "the shell's; in the mass it only sets the march's stride, a quality effect that "
                 "will need a dial of its own",
    "distant-count": "the distant nebulae are shells; a mass hides them",
    "distant-min-deg": "as distant-count", "distant-max-deg": "as distant-count"}
for d in ("pillars", "pillar-length", "pillar-width", "pillar-density", "clouds", "cloud-length",
          "cloud-width", "cloud-distance", "cloud-density"):
    SHELL_NOTE[d] = "capsule pillars and clouds, which the mass skips"
INVISIBLE = {"young": "the clusters' young stars", "cluster-stars": "the clusters' lighting stars",
             "external-galaxies": "other galaxies, far off"}
VISIBLE = 0.005   # a class of "no visible effect" must stay under half a percent of the sky


def load(path, seeds=None):
    by = defaultdict(lambda: defaultdict(list))
    for r in csv.DictReader(open(path)):
        if r["dial"] != "(default)" and (seeds is None or r["seed"] in seeds):
            by[r["dial"]][r["to"]].append(r)
    out = {}
    for d, vals in by.items():
        reach = max(statistics.mean(float(r["change"]) for r in rs) for rs in vals.values())
        share = max(statistics.mean(float(r.get("changed_share") or 0) for r in rs) for rs in vals.values())
        out[d] = (reach, share, next(iter(vals.values()))[0]["from"])
    return out


def morris(path):
    e = defaultdict(list)
    for r in csv.DictReader(open(path)):
        if r["dial"] != "(start)":
            e[r["dial"]].append(float(r["change"]))
    return {d: (statistics.mean(v), statistics.pstdev(v)) for d, v in e.items()}


def main():
    mass, shell = load(sys.argv[1]), load(sys.argv[2])
    fine_mass, fine_shell = load(sys.argv[3]), load(sys.argv[4])
    mm = morris(sys.argv[5]) if len(sys.argv) > 6 else {}
    ms = morris(sys.argv[6]) if len(sys.argv) > 6 else {}
    rows = []
    for d in mass:
        if d in QUALITY:
            cls, note = "quality", "trades time for fidelity; studied for cost"
        elif d in DEBUG:
            cls, note = "debug", DEBUG[d]
        elif d in DECIDED:
            cls, note = "off by a decision", DECIDED[d]
        elif d in INVISIBLE:
            cls, note = "no visible effect", INVISIBLE[d]
        elif d in SHELL:
            cls, note = "shell style (retiring)", SHELL_NOTE[d]
        else:
            cls, note = "look", ""
        # Checks that the numbers bear the class out.
        share = max(mass[d][1], shell[d][1], fine_mass.get(d, (0, 0))[1], fine_shell.get(d, (0, 0))[1])
        if cls == "no visible effect" and share > VISIBLE:
            note += f" -- CHECK: {100 * share:.1f}% of the sky changed"
        if cls == "look" and max(mass[d][0], shell[d][0]) < 0.05:
            note += " -- CHECK: no reach in either style"
        rows.append((d, cls, note))
    order = ["look", "shell style (retiring)", "off by a decision", "no visible effect", "debug", "quality"]
    print("| dial | default | class | mass: reach, sky changed | shell: reach, sky changed |"
          + (" mu* mass / shell |" if mm else "") + " note |")
    print("|---|---|---|---|---|" + ("---|" if mm else "") + "---|")
    for d, cls, note in sorted(rows, key=lambda r: (order.index(r[1]), -max(mass[r[0]][0], shell[r[0]][0]))):
        m, s = mass[d], shell[d]
        mu = ""
        if mm:
            a, b = mm.get(d), ms.get(d)
            mu = f" {a[0]:.2f} / {b[0]:.2f} |" if a and b else " -- |"
        print(f"| {d} | {m[2]} | {cls} | {m[0]:.2f}, {100 * m[1]:.0f}% | {s[0]:.2f}, {100 * s[1]:.0f}% |{mu} {note} |")
    counts = defaultdict(int)
    for _, cls, _ in rows:
        counts[cls] += 1
    print()
    print(", ".join(f"{c}: {counts[c]}" for c in order))


main()
