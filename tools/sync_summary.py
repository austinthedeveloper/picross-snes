#!/usr/bin/env python3
"""Says what a web import changed, in words, for the sync PR (ADR-011).

  python3 tools/sync_summary.py [--base REF]

Compares data/pack.json, data/story.json and data/art/ in the working tree
with REF (default HEAD) and prints Markdown: puzzles, murals, chapters,
jobs, fights, scenes and art that were added, changed or retired, and any
tuning constant that moved. Prints nothing when nothing changed.
"""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def at_ref(ref, rel):
    r = subprocess.run(["git", "show", f"{ref}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return json.loads(r.stdout) if r.returncode == 0 else {}


def now(rel):
    path = ROOT / rel
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}


def by_id(items, key="id"):
    return {item[key]: item for item in items or []}


def diff(old, new):
    added = [k for k in new if k not in old]
    removed = [k for k in old if k not in new]
    changed = [k for k in new if k in old and new[k] != old[k]]
    return added, changed, removed


def section(title, old, new, label=lambda k: k):
    added, changed, removed = diff(old, new)
    out = []
    if added:
        out.append(f"- **New {title}** ({len(added)}): " + ", ".join(label(k) for k in added))
    if changed:
        out.append(f"- **Changed {title}** ({len(changed)}): " + ", ".join(label(k) for k in changed))
    if removed:
        out.append(f"- **Removed {title}** ({len(removed)}): " + ", ".join(label(k) for k in removed))
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="HEAD")
    args = parser.parse_args()

    old_pack, new_pack = at_ref(args.base, "data/pack.json"), now("data/pack.json")
    old_story, new_story = at_ref(args.base, "data/story.json"), now("data/story.json")
    lines = []

    op = {p["id"]: p for p in old_pack.get("puzzles", [])}
    np_ = {p["id"]: p for p in new_pack.get("puzzles", [])}
    retired = [k for k in np_ if np_[k].get("retired") and not op.get(k, {}).get("retired")]
    lines += section("puzzles", {k: v for k, v in op.items() if not v.get("retired")},
                     {k: v for k, v in np_.items() if not v.get("retired")},
                     lambda k: np_.get(k, op.get(k, {})).get("name", k))
    if retired:
        lines.append(f"- **Retired puzzles** (kept in their save slots): {', '.join(retired)}")
    lines += section("murals", by_id(old_pack.get("murals")), by_id(new_pack.get("murals")))
    lines += section("collections", by_id(old_pack.get("collections"), "name"),
                     by_id(new_pack.get("collections"), "name"))

    name = lambda items: (lambda k: items.get(k, {}).get("name") or items.get(k, {}).get("title") or k)
    for key, title in (("chapters", "chapters"), ("runs", "jobs"), ("encounters", "encounters"),
                       ("scenes", "scenes"), ("puzzles", "story puzzles"), ("gadgets", "gadgets")):
        o, n = by_id(old_story.get(key)), by_id(new_story.get(key))
        lines += section(title, o, n, name({**o, **n}))
    oc, nc = old_story.get("constants", {}), new_story.get("constants", {})
    for k in nc:
        if k in oc and oc[k] != nc[k]:
            lines.append(f"- **Tuning:** `{k}` {oc[k]} → {nc[k]}")
    ot, nt = old_story.get("tips", {}), new_story.get("tips", {})
    lines += section("power tips", ot, nt)

    art = subprocess.run(["git", "status", "--porcelain", "--untracked-files=all", "--", "data/art"], cwd=ROOT,
                         capture_output=True, text=True).stdout.split("\n")
    art = [line[3:] for line in art if line.strip()]
    if art:
        lines.append(f"- **Art** ({len(art)} files): " + ", ".join(Path(a).stem for a in art[:20])
                     + (" ..." if len(art) > 20 else ""))
    if lines:
        print("\n".join(lines))


if __name__ == "__main__":
    main()
