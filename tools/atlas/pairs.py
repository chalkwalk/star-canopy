#!/usr/bin/env python3
"""Blind pairs between two atlas renders: for each step of the galaxy work, the
atlas as it was against the atlas as it is (PRINCIPLES §1, §2).

  pairs.py BEFORE_DIR AFTER_DIR OUT_DIR

For each sheet in both, one pair: the two skies side by side, which side is
which drawn at random, each shown as its whole sky (Equal Earth, in galactic
coordinates) above the atlas's views toward the galaxy's centre and along its
plane. The
pairs are shuffled; the key goes to OUT_DIR/key.csv, to be opened only when the
scores are in; OUT_DIR/scores.md is the sheet to score on.

Needs Pillow. Output belongs outside the repository.
"""
import os
import random
import sys

from PIL import Image

GAP = 16


def side(path, portrait):
    sheet = Image.open(path)
    w, h = sheet.width, sheet.height
    top = h - 512
    sky = sheet.crop((0, 0, w, top - GAP))
    sky = sky.resize((1024, round(sky.height * 1024 / w)))
    # The atlas's views are 512 across: toward the centre, then away, then along
    # the plane.
    centre = sheet.crop((0, top, 512, top + 512))
    # From the portrait, outside the galaxy, the view down onto it is the one
    # that shows its form.
    x = 1536 if portrait else 1024
    along = sheet.crop((x, top, x + 512, top + 512))
    out = Image.new("RGB", (1024, sky.height + GAP + 512), (5, 5, 5))
    out.paste(sky, (0, 0))
    out.paste(centre, (0, sky.height + GAP))
    out.paste(along, (512, sky.height + GAP))
    return out


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    before, after, out = sys.argv[1:]
    names = sorted(set(os.listdir(before)) & set(os.listdir(after)))
    names = [n for n in names if n.endswith(".png")]
    if not names:
        sys.exit("no sheets in common")
    os.makedirs(out, exist_ok=True)
    # Not reproducible on purpose: a scorer who could rerun the draw could
    # know the key.
    rng = random.SystemRandom()
    rng.shuffle(names)
    with open(os.path.join(out, "key.csv"), "w") as key, \
            open(os.path.join(out, "scores.md"), "w") as scores:
        key.write("pair,sheet,left,right\n")
        scores.write(
            "# Blind pairs: the galaxy\n\n"
            "Each sheet is one seed's galaxy, the nebula off, from one place in it, made two\n"
            "ways, left and right: the whole sky with the galaxy's centre in the middle and\n"
            "its plane across, and below it views 45 degrees across toward the centre and\n"
            "along the plane (from outside the galaxy, down onto it). Which side is which is\n"
            "drawn at random. Do not open key.csv until every line below is filled in.\n\n"
            "For each pair: which you would rather have (L, R or = for no preference), and a\n"
            "note on what differs.\n\n"
            "| pair | rather have | note |\n|---|---|---|\n")
        for i, name in enumerate(names, 1):
            sides = [("before", before), ("after", after)]
            rng.shuffle(sides)
            portrait = name.startswith("portrait")
            left = side(os.path.join(sides[0][1], name), portrait)
            right = side(os.path.join(sides[1][1], name), portrait)
            sheet = Image.new("RGB", (left.width * 2 + 2 * GAP, left.height), (60, 60, 60))
            sheet.paste(left, (0, 0))
            sheet.paste(right, (left.width + 2 * GAP, 0))
            sheet.save(os.path.join(out, f"pair{i:02d}.png"))
            key.write(f"{i},{name[:-4]},{sides[0][0]},{sides[1][0]}\n")
            scores.write(f"| {i} | | |\n")
    print(f"{len(names)} pairs in {out}")


main()
