#!/usr/bin/env python3
"""What each macro does to the sky, from `study macros`.

  macros.py MACROS.csv

For each macro: how far its ends move the sky; which way, on every descriptor,
and on how many seeds; whether it keeps its promise -- moves the descriptor its
name speaks of the way its name says, in order from -1 to 1, on every seed;
what the skies at its ends are like; and which macros move the sky alike.
Standard library only. The descriptors explain, they do not judge
(PRINCIPLES §2): a promise kept here is a promise about a number, and the
names are for blind scoring to confirm.
"""
import csv
import math
import statistics
import sys
from collections import defaultdict

DESCRIPTORS = ["brightness", "contrast", "clear", "opaque", "chroma", "detail", "bright_share"]
SHORT = {"brightness": "bright", "contrast": "contr", "clear": "clear", "opaque": "opaque",
         "chroma": "chroma", "detail": "detail", "bright_share": "top10"}

# What each macro's name promises, where a descriptor can see it: the
# descriptor, and the sign of its change from -1 to 1. None: nothing measured
# here sees it; it is for the eye.
PROMISE = {
    "open": ("clear", +1),
    "dense": ("opaque", +1),
    "fragmented": ("clear", +1),
    "detailed": ("detail", +1),
    "billowing": None,
    "crisp": ("detail", +1),
    "grand": ("detail", -1),
    "violent": None,
    "luminous": ("brightness", +1),
    "bright": ("brightness", +1),
    "hazy": ("contrast", -1),
    "vivid": ("chroma", +1),
    "starry": None,
}


def main():
    rows = list(csv.DictReader(open(sys.argv[1])))
    base = {r["seed"]: r for r in rows if r["dial"] == "(default)"}
    spread = {k: statistics.pstdev(float(b[k]) for b in base.values()) or 1e-9 for k in DESCRIPTORS}
    at = defaultdict(lambda: defaultdict(dict))  # macro -> value -> seed -> row
    for r in rows:
        if r["dial"] != "(default)":
            at[r["dial"]][float(r["to"])][r["seed"]] = r
    seeds = sorted(base, key=int)

    print(f"{len(seeds)} seeds. The seeds' own skies: clear "
          f"{min(float(b['clear']) for b in base.values()):.0%}-{max(float(b['clear']) for b in base.values()):.0%}, "
          f"brightness {min(float(b['brightness']) for b in base.values()):.2f}-"
          f"{max(float(b['brightness']) for b in base.values()):.2f}.")
    print()
    print("| macro | reach at -1 / 1 | sky changed at -1 / 1 | from -1 to 1 | promise | kept on |")
    print("|---|---|---|---|---|---|")
    vectors = {}
    for m, by in at.items():
        reach = {v: statistics.mean(float(r["change"]) for r in by[v].values()) for v in (-1.0, 1.0)}
        share = {v: statistics.mean(float(r["changed_share"]) for r in by[v].values()) for v in (-1.0, 1.0)}
        moves, vec = [], []
        for k in DESCRIPTORS:
            per = [float(by[1.0][s][k]) - float(by[-1.0][s][k]) for s in seeds]
            mean = statistics.mean(per) / spread[k]
            vec.append(mean)
            if abs(mean) > 0.25:
                agree = sum(1 for e in per if (e > 0) == (mean > 0))
                moves.append(f"{SHORT[k]}{'+' if mean > 0 else '-'}({agree}/{len(per)})")
        vectors[m] = vec
        promise, kept = PROMISE.get(m), "by eye"
        if promise:
            k, sign = promise
            ordered = 0
            for s in seeds:
                xs = [sign * float(by[v][s][k]) for v in (-1.0, -0.5, 0.5, 1.0)]
                xs.insert(2, sign * float(base[s][k]))
                ordered += all(a < b for a, b in zip(xs, xs[1:]))
            kept = f"{ordered}/{len(seeds)} in order"
            promise = f"{SHORT[k]}{'+' if sign > 0 else '-'}"
        print(f"| {m} | {reach[-1.0]:.1f} / {reach[1.0]:.1f} | {share[-1.0]:.0%} / {share[1.0]:.0%} | "
              f"{' '.join(moves)} | {promise or '--'} | {kept} |")

    print()
    print("The skies at the ends, over the seeds (min-median-max):")
    print()
    print("| macro | clear at -1 | clear at 1 | brightness at -1 | brightness at 1 |")
    print("|---|---|---|---|---|")
    for m, by in at.items():
        def span(v, k, pct):
            xs = sorted(float(r[k]) for r in by[v].values())
            f = (lambda x: f"{x:.0%}") if pct else (lambda x: f"{x:.2f}")
            return f"{f(xs[0])}-{f(statistics.median(xs))}-{f(xs[-1])}"
        print(f"| {m} | {span(-1.0, 'clear', True)} | {span(1.0, 'clear', True)} | "
              f"{span(-1.0, 'brightness', False)} | {span(1.0, 'brightness', False)} |")

    print()
    print("Macros that move the descriptors alike (cosine over the seven, from -1 to 1, over 0.8):")
    print()
    names = list(vectors)
    alike = []
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            va, vb = vectors[a], vectors[b]
            na, nb = math.sqrt(sum(x * x for x in va)), math.sqrt(sum(x * x for x in vb))
            if na and nb:
                c = sum(x * y for x, y in zip(va, vb)) / (na * nb)
                if abs(c) > 0.8:
                    alike.append((abs(c), a, b, c))
    for _, a, b, c in sorted(alike, reverse=True):
        print(f"- {a} and {b}: {c:+.2f}")
    if not alike:
        print("- none")


main()
