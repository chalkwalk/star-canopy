#!/usr/bin/env python3
"""Summarise the parameter study's CSVs as Markdown tables.

  analyse.py oat OAT.csv          one table: each dial alone, from the defaults
  analyse.py morris MORRIS.csv    one table: each dial across the whole space
  analyse.py both OAT.csv MORRIS.csv
                                  the two side by side, and each dial's class

Standard library only. The numbers explain how dials move the sky; none says
better (PRINCIPLES §2).
"""
import csv
import math
import statistics
import sys
from collections import defaultdict

DESCRIPTORS = ["brightness", "contrast", "clear", "opaque", "chroma", "detail", "bright_share"]
SHORT = {"brightness": "bright", "contrast": "contr", "clear": "clear", "opaque": "opaque",
         "chroma": "chroma", "detail": "detail", "bright_share": "top10"}
# Below this, in 8-bit display levels averaged over the sky, a dial does
# nothing: exactly 0 where its value is never read; floating-point noise
# otherwise.
DEAD = 0.05


def number(text):
    try:
        return float(text)
    except ValueError:
        return None


def load(path):
    with open(path) as f:
        return list(csv.DictReader(f))


def oat(path):
    rows = load(path)
    base = {r["seed"]: r for r in rows if r["dial"] == "(default)"}
    # The spread of each descriptor between seeds: the scale an effect is
    # judged against, so seed noise cannot pass for one.
    spread = {k: statistics.pstdev(float(b[k]) for b in base.values()) or 1e-9 for k in DESCRIPTORS}
    by = defaultdict(lambda: defaultdict(list))
    for r in rows:
        if r["dial"] != "(default)":
            by[r["dial"]][r["to"]].append(r)
    out = {}
    for dial, values in by.items():
        changes = {v: statistics.mean(float(r["change"]) for r in rs) for v, rs in values.items()}
        reach_value = max(changes, key=changes.get)
        share = None
        if "changed_share" in rows[0]:
            share = max(statistics.mean(float(r["changed_share"]) for r in rs) for rs in values.values())
        numeric = all(number(v) is not None for v in values)
        default = number(next(iter(values.values()))[0]["from"])
        direction = {}
        consistent = {}
        if numeric and default is not None:
            # Up: values above the default against those below it; the effect
            # of turning the dial up, per seed, in units of seed spread.
            for k in DESCRIPTORS:
                per_seed = []
                for seed, b in base.items():
                    hi = [float(r[k]) - float(b[k]) for v, rs in values.items() if number(v) > default
                          for r in rs if r["seed"] == seed]
                    lo = [float(r[k]) - float(b[k]) for v, rs in values.items() if number(v) < default
                          for r in rs if r["seed"] == seed]
                    effect = (statistics.mean(hi) if hi else 0.0) - (statistics.mean(lo) if lo else 0.0)
                    per_seed.append(effect)
                mean = statistics.mean(per_seed) / spread[k]
                direction[k] = "+" if mean > 0.25 else "-" if mean < -0.25 else ""
                if direction[k]:
                    agree = sum(1 for e in per_seed if (e > 0) == (mean > 0))
                    consistent[k] = f"{agree}/{len(per_seed)}"
        seconds = statistics.mean(float(r["seconds"]) for rs in values.values() for r in rs)
        out[dial] = {"reach": changes[reach_value], "at": reach_value, "share": share,
                     "direction": direction,
                     "consistent": consistent, "seconds": seconds,
                     "from": next(iter(values.values()))[0]["from"]}
    return out


def morris(path):
    rows = load(path)
    effects = defaultdict(list)
    for r in rows:
        if r["dial"] not in ("(start)",):
            effects[r["dial"]].append(float(r["change"]))
    return {d: {"mu": statistics.mean(e), "sigma": statistics.pstdev(e), "n": len(e)}
            for d, e in effects.items()}


def arrows(entry):
    parts = []
    for k in DESCRIPTORS:
        d = entry["direction"].get(k, "")
        if d:
            parts.append(f"{SHORT[k]}{d}({entry['consistent'][k]})")
    return " ".join(parts)


def classify(o, m):
    """A dial's class from the numbers alone; the write-up may overrule it,
    with a reason."""
    reach = o["reach"] if o else 0.0
    mu = m["mu"] if m else 0.0
    if reach < DEAD and mu < DEAD:
        return "dead"
    if reach < DEAD <= mu:
        return "only together"
    if reach < 1.0 and mu < 1.0:
        return "slight"
    return "live"


def main():
    mode = sys.argv[1]
    if mode == "oat":
        res = oat(sys.argv[2])
        shares = any(e["share"] is not None for e in res.values())
        print("| dial | default | reach | at |" + (" sky visibly changed |" if shares else "") +
              " turning it up moves | s/bake |")
        print("|---|---|---|---|" + ("---|" if shares else "") + "---|---|")
        for d, e in sorted(res.items(), key=lambda x: -x[1]["reach"]):
            share = f" {100 * e['share']:.1f}% |" if shares else ""
            print(f"| {d} | {e['from']} | {e['reach']:.2f} | {e['at']} |{share} {arrows(e)} | {e['seconds']:.1f} |")
    elif mode == "morris":
        res = morris(sys.argv[2])
        print("| dial | mu* | sigma | steps |")
        print("|---|---|---|---|")
        for d, e in sorted(res.items(), key=lambda x: -x[1]["mu"]):
            print(f"| {d} | {e['mu']:.2f} | {e['sigma']:.2f} | {e['n']} |")
    elif mode == "both":
        o, m = oat(sys.argv[2]), morris(sys.argv[3])
        print("| dial | class | reach from defaults | mu* anywhere | sigma | turning it up moves |")
        print("|---|---|---|---|---|---|")
        names = sorted(set(o) | set(m), key=lambda d: -max(o.get(d, {}).get("reach", 0), m.get(d, {}).get("mu", 0)))
        for d in names:
            oe, me = o.get(d), m.get(d)
            print(f"| {d} | {classify(oe, me)} | {oe['reach']:.2f} | " if oe else f"| {d} | {classify(oe, me)} | -- | ", end="")
            print(f"{me['mu']:.2f} | {me['sigma']:.2f} | " if me else "-- | -- | ", end="")
            print(f"{arrows(oe) if oe else ''} |")
    else:
        sys.exit(__doc__)


main()
