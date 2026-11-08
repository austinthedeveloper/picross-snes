#!/usr/bin/env python3
"""Writes a .pokes file that sets a board up: solved but for a few cells.

  python3 tools/board_pokes.py (--slot N | --encounter ID) --frame F
                               [--leave K] [--out test/smoke/NAME.pokes]

Fills every filled cell of the puzzle except the first K (default 1),
keeps g_right_filled in step so the game's solve check works, and redraws
the board (g_board_dirty_all). Poke at a frame after the play screen has
opened: entering it clears the board. It prints, for the timeline, where
the cells left are and the d-pad presses to reach the first from the top
left (four frames apart, starting at --moves-from).

--slot is a pack slot (data/pack.json); --encounter a story encounter id
(data/story.json), whose puzzle may be a pack or a story puzzle. A fight's
puzzle as drawn: a turned board (the Weathervane) isn't handled.
"""
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def puzzle_rows(slot, encounter):
    pack = json.loads((ROOT / "data/pack.json").read_text(encoding="utf-8"))
    if slot is not None:
        return pack["puzzles"][slot]["rows"], f"pack slot {slot}"
    story = json.loads((ROOT / "data/story.json").read_text(encoding="utf-8"))
    enc = {e["id"]: e for e in story["encounters"]}.get(encounter)
    if enc is None:
        sys.exit(f"no encounter {encounter}")
    pid = enc["puzzle"]
    for p in story["puzzles"]:
        if p["id"] == pid:
            return p["rows"], f"encounter {encounter} (story puzzle {pid})"
    for p in pack["puzzles"]:
        if p["id"] == pid:
            return p["rows"], f"encounter {encounter} (pack puzzle {pid})"
    sys.exit(f"no puzzle {pid}")


def main():
    parser = argparse.ArgumentParser()
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--slot", type=int)
    src.add_argument("--encounter")
    parser.add_argument("--frame", type=int, required=True)
    parser.add_argument("--leave", type=int, default=1)
    parser.add_argument("--out")
    parser.add_argument("--moves-from", type=int, help="first frame of the d-pad presses")
    args = parser.parse_args()

    rows, what = puzzle_rows(args.slot, args.encounter)
    w = len(rows[0])
    cells = [1 if ch == "#" else 0 for r in rows for ch in r]
    filled = [i for i, v in enumerate(cells) if v]
    left, poke = filled[:args.leave], filled[args.leave:]
    lines = [f"# {what}: solved but for {len(left)} cell(s).",
             f"# python3 tools/board_pokes.py "
             + (f"--slot {args.slot}" if args.slot is not None else f"--encounter {args.encounter}")
             + f" --frame {args.frame} --leave {args.leave}"]
    lines += [f"{args.frame} g_cells+{i} 1 1" for i in poke]
    lines.append(f"{args.frame} g_right_filled 2 {len(poke)}")
    lines.append(f"{args.frame} g_board_dirty_all 1 1")
    text = "\n".join(lines) + "\n"
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
        print(f"wrote {args.out}", file=sys.stderr)
    else:
        sys.stdout.write(text)

    r, c = divmod(left[0], w)
    print(f"left: " + ", ".join(f"row {i // w} col {i % w}" for i in left), file=sys.stderr)
    if args.moves_from is not None:
        f = args.moves_from
        out = []
        for _ in range(r):
            out.append(f"{f} {f + 1} down")
            f += 4
        for _ in range(c):
            out.append(f"{f} {f + 1} right")
            f += 4
        out.append(f"{f} {f + 1} a")
        print("timeline (the first press after a screen opens can be swallowed):",
              file=sys.stderr)
        print("\n".join(out), file=sys.stderr)


if __name__ == "__main__":
    main()
