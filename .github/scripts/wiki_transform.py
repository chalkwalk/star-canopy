#!/usr/bin/env python3
"""Turn the Docusaurus docs into a GitHub wiki.

The wiki is a flat set of pages with no frontmatter, no admonition syntax and no
relative .md links, so this rewrites all three. Page order comes from each
file's own `sidebar_position` rather than from sidebars.ts: the sidebar is
TypeScript with nested categories, and regexing it was fragile in a way that
failed silently -- a page quietly missing from the wiki index is worse than a
crash.

Usage: wiki_transform.py <wiki-checkout-dir>
"""

import os
import re
import sys

# The site's own files -- the gallery's images and project files -- are served
# from the site, and the wiki has none of them: their paths become the site's.
SITE = "https://canopy.chalkwalkmusic.com"
SITE_PATH = re.compile(r"""(\]\(|src=["'])(?:pathname://)?/(img|gallery)/""")

# Docusaurus admonitions (:::note ... :::) render as literal colons on the
# wiki. Blockquote them instead, keeping the kind as a bold lead-in.
ADMONITION_OPEN = re.compile(r"^:::(note|tip|info|warning|caution|danger)\s*(.*)$")


def split_frontmatter(text):
    """Return (frontmatter dict, body). Frontmatter is flat key: value."""
    if not text.startswith("---\n"):
        return {}, text
    end = text.find("\n---\n", 4)
    if end == -1:
        return {}, text
    raw = text[4:end]
    body = text[end + 5 :]
    meta = {}
    for line in raw.split("\n"):
        if ":" in line:
            k, _, v = line.partition(":")
            meta[k.strip()] = v.strip().strip("'\"")
    return meta, body


def convert_admonitions(body):
    out = []
    in_admonition = False
    for line in body.split("\n"):
        if in_admonition:
            if line.strip() == ":::":
                in_admonition = False
                out.append("")
                continue
            out.append("> " + line if line.strip() else ">")
            continue
        m = ADMONITION_OPEN.match(line)
        if m:
            kind, title = m.group(1), m.group(2).strip()
            heading = title if title else kind.capitalize()
            out.append("> **{}**".format(heading))
            out.append(">")
            in_admonition = True
            continue
        out.append(line)
    return "\n".join(out)


def convert_links(body, page_titles):
    """`[text](./target.md)` -> `[text](Target-Page)`; intro becomes Home."""

    def repl(match):
        text, target = match.group(1), match.group(2)
        return "[{}]({})".format(text, "Home" if target == "intro" else target)

    return re.sub(
        r"\[([^\]]+)\]\((?:\./|\.\./docs/)?([a-zA-Z0-9_-]+)\.md\)", repl, body
    )


def convert_site_paths(body):
    """`](/img/x.jpg)` and `src="/img/x.jpg"` -> the same on the site."""
    return SITE_PATH.sub(lambda m: "{}{}/{}/".format(m.group(1), SITE, m.group(2)), body)


def main(wiki_dir):
    pages = []
    for name in sorted(os.listdir(wiki_dir)):
        if not name.endswith(".md") or name.startswith("_"):
            continue
        path = os.path.join(wiki_dir, name)
        with open(path, encoding="utf-8") as f:
            meta, body = split_frontmatter(f.read())
        slug = meta.get("id") or os.path.splitext(name)[0]
        try:
            position = int(meta.get("sidebar_position", "999"))
        except ValueError:
            position = 999
        pages.append(
            {
                "path": path,
                "slug": slug,
                "title": meta.get("title") or slug.replace("-", " ").capitalize(),
                "position": position,
            }
        )

    if not pages:
        sys.exit("no markdown pages found in {}".format(wiki_dir))

    titles = {p["slug"]: p["title"] for p in pages}

    for page in pages:
        with open(page["path"], encoding="utf-8") as f:
            _, body = split_frontmatter(f.read())
        body = convert_admonitions(body)
        body = convert_links(body, titles)
        body = convert_site_paths(body)
        body = body.lstrip("\n")

        target = page["path"]
        if page["slug"] == "intro":
            target = os.path.join(wiki_dir, "Home.md")
            os.remove(page["path"])
        page["wiki_name"] = os.path.splitext(os.path.basename(target))[0]

        with open(target, "w", encoding="utf-8") as f:
            f.write(body)
        print("wrote {}".format(os.path.basename(target)))

    pages.sort(key=lambda p: (p["position"], p["slug"]))
    sidebar = os.path.join(wiki_dir, "_Sidebar.md")
    with open(sidebar, "w", encoding="utf-8") as f:
        f.write("### StarCanopy\n\n")
        for page in pages:
            f.write("* [{}]({})\n".format(page["title"], page["wiki_name"]))
        f.write(
            "\n---\n\n"
            "This wiki is generated from `website/docs/` in the main repository.\n"
            "Edits made here are overwritten on the next sync; change the source\n"
            "instead.\n"
        )
    print("wrote _Sidebar.md with {} entries".format(len(pages)))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
