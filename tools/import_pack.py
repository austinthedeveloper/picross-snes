#!/usr/bin/env python3
"""Imports the free-play pack from the web game into data/pack.json.

  python3 tools/import_pack.py [--from PATH_TO_WEB_REPO]

The web repo (default ../../picross, the sibling checkout) is the source of
truth for the drawings. This copies, for every pack puzzle: its id, name,
size, rows, difficulty rating (solver passes), and its collection (the theme
comments in src/puzzles/index.ts's ORDER).

Murals (src/puzzles/murals/*.json): each picture splits into tile x tile
puzzles, row by row, imported as pack puzzles "<mural>-<n>" in collection
"Murals" (not a real collection: the game lists them on the murals screen),
plus a "murals" list giving each mural's size in tiles and its tile ids.

Save slots (ADR-003) are append-only: a puzzle keeps the slot it was first
imported with, forever, because the battery save indexes records by slot.
New puzzles go on the end; a puzzle removed from the web pack keeps its slot
with "retired": true so old saves still line up.
"""
import argparse
import json
import re
import sys
from pathlib import Path

from web_paths import src as web_src

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data" / "pack.json"


# Web collection names too long for the SNES list (COLLECTION_NAME_MAX in
# tools/gen_assets.py), and what the cartridge calls them instead.
SHORT_NAMES = {
    "Pets and instruments": "Pets & instruments",
}


def collection_name(comment):
    """A theme comment's collection name: the text before any note in
    parentheses or after a colon ("Pets and instruments (20x20), insane: ...")."""
    name = re.split(r"\s\(|:", comment, maxsplit=1)[0].strip()
    return SHORT_NAMES.get(name, name)


def string_array(ts, name):
    """The ids in `const NAME ... = [ 'a', 'b', ... ]` in a .ts file's text."""
    m = re.search(rf"const\s+{name}\b[^=]*=\s*\[", ts)
    if not m:
        raise SystemExit(f"ORDER spreads {name}, which isn't a string array")
    return re.findall(r"'([^']+)'", ts[m.end():ts.index("]", m.end())])


def spread_ids(web, index_ts, name):
    """The ids of `...NAME` in ORDER: an array in index.ts itself, or imported
    from another package (`import { NAME } from '@picross/shared/puzzles/pairs'`)."""
    imp = re.search(rf"import\s*{{[^}}]*\b{name}\b[^}}]*}}\s*from\s*'@picross/([^/']+)/([^']+)'",
                    index_ts)
    if not imp:
        return string_array(index_ts, name)
    path = Path(web) / "packages" / imp.group(1) / "src" / f"{imp.group(2)}.ts"
    return string_array(path.read_text(encoding="utf-8"), name)


def parse_order(index_ts, web=None):
    """[(collection name, [ids...]), ...] in ORDER, from the theme comments."""
    start = index_ts.index("const ORDER = [")
    body = index_ts[start:index_ts.index("];", start)]
    groups = []
    for line in body.splitlines():
        comment = re.match(r"\s*// (.*)", line)
        if comment:
            groups.append((collection_name(comment.group(1)), []))
            continue
        item = re.match(r"\s*'([^']+)'", line)
        spread = re.match(r"\s*\.\.\.(\w+)", line)
        if item or spread:
            if not groups:
                raise SystemExit("ORDER starts without a theme comment")
            if item:
                groups[-1][1].append(item.group(1))
            else:
                groups[-1][1].extend(spread_ids(web, index_ts, spread.group(1)))
    return groups


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--from", dest="src", default=str(ROOT.parent.parent / "picross"))
    args = parser.parse_args()
    web = Path(args.src)
    index_ts = web_src(web, "puzzles/index.ts").read_text(encoding="utf-8")
    ratings = json.loads(web_src(web, "puzzles/ratings.json").read_text(encoding="utf-8"))
    groups = parse_order(index_ts, web)

    pack = {}
    for path in sorted(web_src(web, "puzzles/pack").glob("*.json")):
        p = json.loads(path.read_text(encoding="utf-8"))
        pack[p["id"]] = p

    previous = {}
    if OUT.exists():
        for i, p in enumerate(json.loads(OUT.read_text(encoding="utf-8"))["puzzles"]):
            previous[p["id"]] = p
            if p["slot"] != i:
                raise SystemExit(f"{OUT}: slot {p['slot']} out of place at {i}")

    collections = []
    collection_of = {}
    for name, ids in groups:
        collections.append({"name": name, "puzzles": ids})
        for pid in ids:
            if pid not in pack:
                raise SystemExit(f"ORDER names {pid}, which has no pack file")
            collection_of[pid] = name
    loose = sorted(set(pack) - set(collection_of))
    if loose:
        collections.append({"name": "Extras", "puzzles": loose})
        for pid in loose:
            collection_of[pid] = "Extras"

    murals = []
    for path in web_src(web, "puzzles/murals").glob("*.json"):
        m = json.loads(path.read_text(encoding="utf-8"))
        t, rows = m["tile"], m["solution"]
        across, down = len(rows[0]) // t, len(rows) // t
        ids = []
        for n in range(across * down):
            tr, tc = divmod(n, across)
            tid = f"{m['id']}-{n + 1}"
            pack[tid] = {
                "id": tid, "name": m["name"], "width": t, "height": t,
                "solution": [r[tc * t:(tc + 1) * t] for r in rows[tr * t:(tr + 1) * t]],
            }
            collection_of[tid] = "Murals"
            ids.append(tid)
        murals.append({"id": m["id"], "name": m["name"], "order": m["order"],
                       "across": across, "down": down, "tiles": ids})
    murals.sort(key=lambda m: m["order"])

    # Slots: keep every old one, append new ids: collections in ORDER, then
    # the murals' tiles.
    slots = [p["id"] for p in previous.values()]
    for pid in [pid for c in collections for pid in c["puzzles"]] + \
            [tid for m in murals for tid in m["tiles"]]:
        if pid not in previous:
            slots.append(pid)

    puzzles = []
    for slot, pid in enumerate(slots):
        if pid not in pack:
            old = dict(previous[pid])
            old["retired"] = True
            puzzles.append(old)
            continue
        p = pack[pid]
        if pid in previous and previous[pid]["rows"] != p["solution"]:
            print(f"warning: {pid}'s picture changed; a published picture should "
                  "be frozen (docs/PUZZLES.md)", file=sys.stderr)
        puzzles.append({
            "slot": slot,
            "id": pid,
            "name": p["name"],
            "width": p["width"],
            "height": p["height"],
            "passes": ratings.get(pid, 0),
            "collection": collection_of[pid],
            "rows": p["solution"],
        })

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({
        "source": "picross web game, src/puzzles (tools/import_pack.py)",
        "collections": collections,
        "murals": murals,
        "puzzles": puzzles,
    }, indent=1) + "\n", encoding="utf-8")
    print(f"{OUT.relative_to(ROOT)}: {len(puzzles)} puzzles in {len(collections)} "
          f"collections, {len(murals)} murals")


if __name__ == "__main__":
    main()
