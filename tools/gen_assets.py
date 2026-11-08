#!/usr/bin/env python3
"""Builds every generated asset into assets/generated/ (make assets).

  python3 tools/gen_assets.py [--check]

Inputs: data/pack.json (tools/import_pack.py) and the art defined in code
here and in tools/font.py. Outputs (never committed):

  font.pic         BG3 text font, 2bpp, ASCII 32..127 (tile = char - 32)
  board.pic        BG1 tiles, 4bpp: cells, clues, frames, bars, medals
  obj.pic          sprite tiles, 4bpp: the board cursor
  palette.pal      all of CGRAM (256 colours, BGR555)
  sfx.it, sfx.h    the sound effects (tools/sfx.py), for smconv
  pack_*.bin       the puzzle pack and murals (docs/PUZZLES.md has the layout)
  tiles.h          tile and palette numbers for the C code
  big_tiles.h      the 16-pixel cells' and clues' tile tables
  pack.h           pack counts and limits
  pack_data.c      the pack .bin files as C arrays, for the host tests

--check validates the pack against the limits the game assumes (sizes,
clue counts, name lengths, glyphs, save slots) and exits non-zero on any
problem without writing anything.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from font import ACCENT, GLYPHS, LOGO_FONT, LOWER, NARROW, STORY_GLYPHS  # noqa: E402
from sfx import sfx_assets  # noqa: E402

import gen_art  # noqa: E402
import gen_story  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "generated"
PACK = ROOT / "data" / "pack.json"

# Save capacity (include/save.h SAVE_SLOTS): the pack may grow to this many
# puzzles before the save layout has to change (ADR-003).
SAVE_SLOTS = 700
# pack_info's collection byte for a mural tile.
MURAL_COLLECTION = 255
MAX_SIDE = 20
NAME_MAX = 20
COLLECTION_NAME_MAX = 19
# Clue room per board size (docs/LAYOUT.md): row clues, column clues.
# 5x5 and 10x10 use 16-pixel cells and clues, so they have less room.
CLUE_ROOM = {5: (3, 3), 10: (4, 3), 15: (8, 8), 20: (9, 8)}
BIG_MAX_CLUE = 10
# BG1 tiles fill VRAM words $0000-$3FFF: 1024 4bpp tiles.
BG1_TILE_LIMIT = 1024

# ---------------------------------------------------------------- colours
# The web game's light theme tokens (src/styles/tokens.css).
C = {
    "paper": 0xF6F3EC, "surface": 0xFFFFFF, "ink": 0x23262F, "muted": 0x7A7E8A,
    "line": 0xD6D2C8, "guide": 0x8A8679, "accent": 0xD9534F, "ok": 0x2E8B57,
    "hl": 0xFBEFC9, "cellbg": 0xFFFDF8, "mark": 0xB0AA9C, "backdrop": 0xE2DED3,
    "bronze": 0xA8672F, "silver": 0x7F8A96, "gold": 0xC99A06,
    "bronze_dk": 0x6E4320, "silver_dk": 0x525A63, "gold_dk": 0x8A6A04,
    "bronze_lt": 0xD69A62, "silver_lt": 0xB9C2CC, "gold_lt": 0xF0CC4A,
    # The striped clue lines (every other line, so neighbouring two-digit
    # clues don't run together).
    "paper2": 0xE9E4D7, "hl2": 0xF4E2AE,
}


def bgr555(rgb):
    r, g, b = (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def build_palette():
    cg = [0] * 256

    def bg3(pal, c1, c2, c3):
        for i, name in enumerate((c1, c2, c3), start=1):
            cg[pal * 4 + i] = bgr555(C[name])

    cg[0] = bgr555(C["paper"])                     # the backdrop
    bg3(0, "ink", "line", "accent")                # text
    bg3(1, "muted", "line", "ok")                  # dim text
    bg3(2, "accent", "line", "gold")               # selected text
    bg3(3, "surface", "ink", "accent")             # text on dark
    bg3(4, "ok", "line", "ok")
    bg3(5, "bronze", "line", "bronze")
    bg3(6, "silver", "line", "silver")
    bg3(7, "gold", "line", "gold")

    base = {
        1: "cellbg", 2: "line", 3: "guide", 4: "ink", 5: "mark", 6: "paper",
        7: "paper2", 8: "surface", 9: "guide", 10: "accent", 11: "ok",
        12: "guide", 13: "line", 14: "backdrop", 15: "line",
    }
    medal = {
        3: ("bronze_dk", "bronze_lt"), 4: ("silver_dk", "silver_lt"),
        5: ("gold_dk", "gold_lt"),
    }
    for pal in range(2, 8):
        cols = dict(base)
        if pal in (3, 5, 7):                       # highlighted line
            cols[1] = "hl"
            cols[6] = "hl"
            cols[7] = "hl2"
        if pal in (4, 5):                          # mistake X
            cols[5] = "accent"
        if pal in (6, 7):                          # finished clue
            cols[4] = "muted"
        if pal in medal:                           # a medal's tier
            cols[12], cols[13] = medal[pal]
            cols[14], cols[15] = "cellbg", "ink"
        for i, name in cols.items():
            cg[pal * 16 + i] = bgr555(C[name])
    for i, name in {1: "accent", 2: "ink", 3: "surface", 4: "hl"}.items():
        cg[128 + i] = bgr555(C[name])              # OBJ palette 0
    return b"".join(c.to_bytes(2, "little") for c in cg)


# ---------------------------------------------------------------- tiles
def blank():
    return [[0] * 8 for _ in range(8)]


def encode(tile, bpp):
    out = bytearray()
    for plane in range(0, bpp, 2):
        for y in range(8):
            lo = hi = 0
            for x in range(8):
                v = tile[y][x]
                lo |= ((v >> plane) & 1) << (7 - x)
                hi |= ((v >> (plane + 1)) & 1) << (7 - x)
            out += bytes((lo, hi))
    return bytes(out)


def stamp(tile, rows, x0, y0, colour):
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == "#":
                tile[y0 + y][x0 + x] = colour


def font_tiles():
    """BG3 tiles: ASCII 32-127, then the story glyphs (codes 0x80 on), so a
    character's tile is always its code - 32 (map_draw.asm)."""
    data = bytearray()
    story = dict(STORY_GLYPHS)
    for code in range(32, 0x80 + len(STORY_GLYPHS)):
        t = blank()
        ch = chr(code)
        glyph = (LOWER.get(ch) or GLYPHS.get(ch.upper())) if "a" <= ch <= "z" else GLYPHS.get(ch)
        if code >= 0x80:
            glyph = story[gen_story.GLYPHS[code - 0x80]]
        if glyph:
            ink = 3 if ch in ACCENT else 1
            for y, row in enumerate(glyph):          # drop shadow first
                for x, ch in enumerate(row):
                    if ch == "#":
                        t[y + 1][x + 2] = 2
            stamp(t, glyph, 1, 0, ink)
        data += encode(t, 2)
    return bytes(data)


class Tiles:
    """BG1 tiles in order, with names for tiles.h."""

    def __init__(self):
        self.data = bytearray()
        self.names = {}
        self.count = 0
        self.seen = {}

    def add(self, tile, name=None):
        if name:
            self.names[name] = self.count
        self.data += encode(tile, 4)
        self.count += 1
        return self.count - 1

    def add_shared(self, tile):
        """A tile, reusing an identical one added this way before."""
        key = encode(tile, 4)
        if key not in self.seen:
            self.seen[key] = self.add(tile)
        return self.seen[key]

    def add_big(self, px):
        """A 16x16 picture as four shared tiles: TL, TR, BL, BR."""
        return [self.add_shared([row[tx * 8:tx * 8 + 8] for row in px[ty * 8:ty * 8 + 8]])
                for ty in range(2) for tx in range(2)]

    def pad_to(self, n):
        while self.count < n:
            self.add(blank())


def cell_tile(state, left_guide, top_guide, bottom=False):
    """A cell draws its own top and left grid lines; the bottom row of a
    board also draws the board's bottom edge (20x20 has no room for an
    edge row below it)."""
    t = blank()
    for y in range(8):
        for x in range(8):
            if bottom and y == 7:
                t[y][x] = 3
            elif y == 0 or x == 0:
                guide = (y == 0 and top_guide) or (x == 0 and left_guide)
                t[y][x] = 3 if guide else 2
            elif state == 1:
                t[y][x] = 4
            else:
                t[y][x] = 1
    if state == 2:
        for i in range(5):
            y = 2 + i if not bottom else 1 + i
            t[y][2 + i] = 5
            t[y][6 - i] = 5
    return t


def big_cell(state, left_guide, top_guide, bottom):
    """A 16-pixel cell (5x5 and 10x10 boards), drawn like cell_tile."""
    px = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            if bottom and y == 15:
                px[y][x] = 3
            elif y == 0 or x == 0:
                guide = (y == 0 and top_guide) or (x == 0 and left_guide)
                px[y][x] = 3 if guide else 2
            elif state == 1:
                px[y][x] = 4
            else:
                px[y][x] = 1
    if state == 2:
        top = 4 if not bottom else 3
        for i in range(8):
            for d in (0, 1):
                y = top + i
                if 4 + i + d < 16:
                    px[y][4 + i + d] = 5
                if 11 - i + d < 16:
                    px[y][11 - i + d] = 5
    return px


def scale2(rows):
    return ["".join(ch * 2 for ch in row) for row in rows for _ in range(2)]


def big_clue(n, bg):
    """A 16x16 clue: the 5x7 font at double size (10 in narrow digits)."""
    px = [[bg] * 16 for _ in range(16)]

    def put(rows, x0, y0):
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch == "#":
                    px[y0 + y][x0 + x] = 4
    s = str(n)
    if len(s) == 1:
        put(scale2(GLYPHS[s]), 4, 2)
    else:
        put(scale2(NARROW[s[0]]), 1, 2)
        put(scale2(NARROW[s[1]]), 9, 2)
    return px


def clue_tile(n, bg=6):
    t = [[bg] * 8 for _ in range(8)]
    s = str(n)
    if len(s) == 1:
        stamp(t, GLYPHS[s], 2, 1, 4)
    else:
        stamp(t, NARROW[s[0]], 0, 1, 4)
        stamp(t, NARROW[s[1]], 4, 1, 4)
    return t


def frame_tiles():
    """A rounded card: 9 tiles TL T TR L C R BL B BR."""
    tiles = []
    for ty in range(3):
        for tx in range(3):
            t = [[8] * 8 for _ in range(8)]
            for y in range(8):
                for x in range(8):
                    edge_x = (tx == 0 and x == 0) or (tx == 2 and x == 7)
                    edge_y = (ty == 0 and y == 0) or (ty == 2 and y == 7)
                    if edge_x or edge_y:
                        t[y][x] = 9
            # round the corners
            if tx != 1 and ty != 1:
                cx = 0 if tx == 0 else 7
                cy = 0 if ty == 0 else 7
                dx = 1 if tx == 0 else -1
                dy = 1 if ty == 0 else -1
                t[cy][cx] = 0
                t[cy][cx + dx] = 0
                t[cy + dy][cx] = 0
                t[cy + dy][cx + dx] = 9
                t[cy][cx + 2 * dx] = 9
                t[cy + 2 * dy][cx] = 9
            tiles.append(t)
    return tiles


def bar_tile(full):
    t = blank()
    for y in range(2, 6):
        for x in range(8):
            t[y][x] = 4 if full else 2
    return t


def medal_tiles(picture):
    """32x32 (4x4 tiles, row-major): a rounded plate holding a 1-bit picture."""
    px = [[0] * 32 for _ in range(32)]
    for y in range(32):
        for x in range(32):
            d = min(x, y, 31 - x, 31 - y)
            corner = min(x, 31 - x) + min(y, 31 - y) < 4
            if corner:
                continue
            px[y][x] = 12 if d == 0 else 13 if d < 3 else 14
    h, w = len(picture), len(picture[0])
    oy, ox = (32 - h) // 2, (32 - w) // 2
    for y, row in enumerate(picture):
        for x, ch in enumerate(row):
            if ch == "#":
                px[oy + y][ox + x] = 15
    tiles = []
    for ty in range(4):
        for tx in range(4):
            tiles.append([[px[ty * 8 + y][tx * 8 + x] for x in range(8)] for y in range(8)])
    return tiles


def board_tiles(collections, puzzles_by_id):
    tiles = Tiles()
    tiles.add(blank(), "T_BLANK")
    for state in range(3):
        for tg in range(2):
            for lg in range(2):
                name = "T_CELL" if (state, tg, lg) == (0, 0, 0) else None
                tiles.add(cell_tile(state, lg, tg), name)
    for state in range(3):
        for lg in range(2):
            tiles.add(cell_tile(state, lg, False, True),
                      "T_CELL_B" if (state, lg) == (0, 0) else None)
    edge_r = blank()
    for y in range(8):
        edge_r[y][0] = 3
    tiles.add(edge_r, "T_EDGE_R")
    edge_rb = blank()
    for y in range(8):
        edge_rb[y][0] = 3
    edge_rb[7][0] = 3
    tiles.add(edge_rb, "T_EDGE_RB")
    edge_b = blank()
    for x in range(8):
        edge_b[0][x] = 3
    tiles.add(edge_b, "T_EDGE_B")
    corner = blank()
    corner[0][0] = 3
    tiles.add(corner, "T_EDGE_C")
    tiles.add([[1] * 8 for _ in range(8)], "T_PIC_EMPTY")
    tiles.add([[4] * 8 for _ in range(8)], "T_PIC_FULL")
    tiles.add([[6] * 8 for _ in range(8)], "T_CLUE_BLANK")
    for n in range(MAX_SIDE + 1):
        tiles.add(clue_tile(n), "T_CLUE" if n == 0 else None)
    tiles.add([[7] * 8 for _ in range(8)], "T_CLUE_BLANK_ALT")
    for n in range(MAX_SIDE + 1):
        tiles.add(clue_tile(n, 7), "T_CLUE_ALT" if n == 0 else None)
    for i, t in enumerate(frame_tiles()):
        tiles.add(t, "T_FRAME" if i == 0 else None)
    tiles.add(bar_tile(False), "T_BAR_EMPTY")
    tiles.add(bar_tile(True), "T_BAR_FULL")
    # 16-pixel cells: [state][variant] -> TL TR BL BR, variant = left guide
    # + (bottom row ? 4 : 2 * top guide) (a bottom row never has a top guide).
    big_cells = []
    for state in range(3):
        for lg, tg, bottom in ((0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0), (0, 0, 1), (1, 0, 1)):
            big_cells.append(tiles.add_big(big_cell(state, lg, tg, bottom)))
    big_clues = []
    for bg in (6, 7):
        for n in range(BIG_MAX_CLUE + 1):
            big_clues.append(tiles.add_big(big_clue(n, bg)))
    tiles.big_cells = big_cells
    tiles.big_clues = big_clues
    while tiles.count % 16:
        tiles.add(blank())
    for i, c in enumerate(collections):
        for j, t in enumerate(medal_tiles(puzzles_by_id[c["emblem"]]["rows"])):
            tiles.add(t, "T_MEDAL" if i == 0 and j == 0 else None)
    return tiles


def obj_tiles():
    """The cursors as 16x16 sprites: an 11x11 two-ring box for 8-pixel cells
    (tiles 0,1,16,17) and a 16x16 one for 16-pixel cells (2,3,18,19)."""
    sheet = [blank() for _ in range(32)]
    for first, size in ((0, 11), (2, 16)):
        px = [[0] * 16 for _ in range(16)]
        for y in range(size):
            for x in range(size):
                d = min(x, y, size - 1 - x, size - 1 - y)
                if d == 0:
                    px[y][x] = 2
                elif d == 1:
                    px[y][x] = 1
        for ty in range(2):
            for tx in range(2):
                sheet[ty * 16 + first + tx] = [[px[ty * 8 + y][tx * 8 + x] for x in range(8)]
                                               for y in range(8)]
    return b"".join(encode(t, 4) for t in sheet)


# ---------------------------------------------------------------- pack
def runs(line):
    out, n = [], 0
    for ch in line:
        if ch == "#":
            n += 1
        elif n:
            out.append(n)
            n = 0
    if n:
        out.append(n)
    return out or [0]


def validate(pack):
    problems = []
    puzzles = [p for p in pack["puzzles"] if not p.get("retired")]
    if len(pack["puzzles"]) > SAVE_SLOTS:
        problems.append(f"{len(pack['puzzles'])} slots; the save holds {SAVE_SLOTS} (ADR-003)")
    if len(pack["collections"]) > 32:
        problems.append("more than 32 collections")
    for p in puzzles:
        w, h, pid = p["width"], p["height"], p["id"]
        if w != h or w not in CLUE_ROOM:
            problems.append(f"{pid}: {w}x{h} has no layout (docs/LAYOUT.md)")
            continue
        if len(p["rows"]) != h or any(len(r) != w for r in p["rows"]):
            problems.append(f"{pid}: rows don't match {w}x{h}")
        row_room, col_room = CLUE_ROOM[w]
        rc = max(len(runs(r)) for r in p["rows"])
        cc = max(len(runs("".join(r[c] for r in p["rows"]))) for c in range(w))
        if rc > row_room or cc > col_room:
            problems.append(f"{pid}: {rc} row / {cc} column clues; room is {row_room}/{col_room}")
        name = p["name"].upper()
        if len(name) > NAME_MAX:
            problems.append(f"{pid}: name longer than {NAME_MAX}")
        bad = sorted({ch for ch in name if ch not in GLYPHS or ch in "{}~`|^[]"})
        if bad:
            problems.append(f"{pid}: name uses {bad}, which the font lacks")
    for m in pack.get("murals", []):
        if len(m["name"]) > COLLECTION_NAME_MAX:
            problems.append(f"mural {m['name']}: name longer than {COLLECTION_NAME_MAX}")
        if m["across"] * m["down"] > 9 or m["across"] > 3 or m["down"] > 3:
            problems.append(f"mural {m['name']}: more than 3x3 tiles (the preview is 48x48)")
    for c in pack["collections"]:
        if len(c["name"]) > COLLECTION_NAME_MAX:
            problems.append(f"collection {c['name']}: name longer than {COLLECTION_NAME_MAX}")
    return problems


def choose_emblem(c, by_id):
    """The collection's medal picture: its first 15x15 puzzle, else its first."""
    for pid in c["puzzles"]:
        if by_id[pid]["width"] == 15:
            return pid
    return c["puzzles"][0]


def u16(n):
    return n.to_bytes(2, "little")


# Story puzzles (ADR-010) sit in the pack tables from this ROM index on, past
# every save slot, so a free-play record can never point at one.
STORY_PUZZLE_BASE = SAVE_SLOTS
STORY_COLLECTION = 254


def pack_bins(pack, by_id, story_puzzles=()):
    info, bits, names = bytearray(), bytearray(), bytearray()
    puzzles = list(pack["puzzles"])
    puzzles += [None] * (STORY_PUZZLE_BASE - len(puzzles))
    puzzles += [{"width": w, "height": h, "passes": 0, "collection": None,
                 "rows": rows, "name": name} for w, h, rows, name in story_puzzles]
    for p in puzzles:
        if p is None:
            info += bytes(8)
            continue
        info += u16(len(bits)) + u16(len(names))
        info += bytes((p["width"], p["height"], min(p["passes"], 255),
                       STORY_COLLECTION if p["collection"] is None else
                       next((i for i, c in enumerate(pack["collections"])
                             if c["name"] == p["collection"]), MURAL_COLLECTION)))
        for row in p["rows"]:
            v = 0
            nbytes = (p["width"] + 7) // 8
            for x, ch in enumerate(row):
                if ch == "#":
                    v |= 1 << (nbytes * 8 - 1 - x)
            bits += v.to_bytes(nbytes, "big")
        names += p["name"].upper().encode("ascii") + b"\0"
    slot_of = {p["id"]: p["slot"] for p in pack["puzzles"]}
    cinfo, cslots, cnames = bytearray(), bytearray(), bytearray()
    for i, c in enumerate(pack["collections"]):
        cinfo += u16(len(cslots) // 2) + bytes((len(c["puzzles"]), i))
        cinfo += u16(len(cnames)) + u16(slot_of[c["emblem"]])
        for pid in c["puzzles"]:
            cslots += u16(slot_of[pid])
        cnames += c["name"].upper().encode("ascii") + b"\0"
    # pack_minfo, 8 bytes a mural: u16 first tile slot (a mural's tiles have
    # consecutive slots), across, down, u16 name offset (in pack_cnames), 0, 0.
    minfo = bytearray()
    for m in pack["murals"]:
        first = slot_of[m["tiles"][0]]
        assert [slot_of[t] for t in m["tiles"]] == list(range(first, first + len(m["tiles"])))
        minfo += u16(first) + bytes((m["across"], m["down"])) + u16(len(cnames)) + bytes(2)
        cnames += m["name"].upper().encode("ascii") + b"\0"
    return {
        "pack_info": bytes(info), "pack_bits": bytes(bits), "pack_names": bytes(names),
        "pack_cinfo": bytes(cinfo), "pack_cslots": bytes(cslots), "pack_cnames": bytes(cnames),
        "pack_minfo": bytes(minfo),
    }


def points(side):
    """Rank points for a solve, by board side (web: src/puzzles/ranks.ts)."""
    if side <= 5:
        return 1
    if side <= 10:
        return 2
    return 3 if side <= 15 else 6


def write_if_changed(path, data):
    if isinstance(data, str):
        data = data.encode("utf-8")
    if path.exists() and path.read_bytes() == data:
        return
    path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    pack = json.loads(PACK.read_text(encoding="utf-8"))
    problems = validate(pack)
    if problems:
        print("data/pack.json:\n  " + "\n  ".join(problems), file=sys.stderr)
        sys.exit(1)
    by_id = {p["id"]: p for p in pack["puzzles"]}
    for c in pack["collections"]:
        c["emblem"] = choose_emblem(c, by_id)
    tiles = board_tiles(pack["collections"], by_id)
    if tiles.count > BG1_TILE_LIMIT:
        print(f"board.pic: {tiles.count} tiles; BG1 has room for {BG1_TILE_LIMIT}",
              file=sys.stderr)
        sys.exit(1)
    if args.check:
        story_bins, _, story = gen_story.build(pack, STORY_PUZZLE_BASE)
        print(f"assets: ok ({len(pack['puzzles'])} puzzles, {tiles.count} BG1 tiles, "
              f"{len(story['encounters'])} encounters, "
              f"{sum(len(b) for b in story_bins.values())} bytes of story)")
        return

    OUT.mkdir(parents=True, exist_ok=True)
    write_if_changed(OUT / "font.pic", font_tiles())
    write_if_changed(OUT / "board.pic", bytes(tiles.data))
    write_if_changed(OUT / "obj.pic", obj_tiles())
    write_if_changed(OUT / "palette.pal", build_palette())
    module, sfx_h, seconds = sfx_assets()
    write_if_changed(OUT / "sfx.it", module)
    write_if_changed(OUT / "sfx.h", sfx_h)
    story_bins, story_h, story = gen_story.build(pack, STORY_PUZZLE_BASE)
    bins = pack_bins(pack, by_id, gen_story.story_puzzle_bins(story))
    for name, data in list(bins.items()) + list(story_bins.items()):
        write_if_changed(OUT / f"{name}.bin", data)
    write_if_changed(OUT / "story_const.h", story_h)
    story_asm, story_c, story_banks = gen_story.asm_and_c(story_bins)
    write_if_changed(OUT / "story_data.asm", story_asm)
    write_if_changed(OUT / "story_data.c", story_c)
    write_if_changed(OUT / "story_banks.h", story_banks)
    art_bins, art_h = gen_art.build()
    for name, data in art_bins.items():
        write_if_changed(OUT / f"{name}.bin", data)
    write_if_changed(OUT / "art_data.h", art_h)
    write_if_changed(OUT / "art_data.asm", gen_art.asm(art_bins))

    h = ["/* Generated by tools/gen_assets.py. Do not edit. */",
         "#ifndef TILES_H", "#define TILES_H"]
    for name, n in tiles.names.items():
        h.append(f"#define {name} {n}")
    h.append(f"#define T_BOARD_COUNT {tiles.count}")
    h += ["/* BG1 palettes (docs/LAYOUT.md) */",
          "#define PAL_NORMAL 2", "#define PAL_HL 3", "#define PAL_MISTAKE 4",
          "#define PAL_MISTAKE_HL 5", "#define PAL_DONE 6", "#define PAL_DONE_HL 7",
          "#define PAL_LOCKED 2", "#define PAL_BRONZE 3", "#define PAL_SILVER 4",
          "#define PAL_GOLD 5", "#endif", ""]
    write_if_changed(OUT / "tiles.h", "\n".join(h))

    def table(name, rows):
        body = ",\n".join("    " + ", ".join(str(t) for t in r) for r in rows)
        return f"static const u16 {name}[{len(rows) * 4}] = {{\n{body}\n}};"
    h = ["/* Generated by tools/gen_assets.py: 16-pixel cells and clues as",
         " * four BG1 tiles each (TL, TR, BL, BR). Include once (board_view.c). */",
         "/* [(state * 6 + variant) * 4]: variant = left guide + (bottom ? 4 : 2 * top guide) */",
         table("s_big_cells", tiles.big_cells),
         f"/* [(striped * {BIG_MAX_CLUE + 1} + n) * 4] */",
         f"#define BIG_MAX_CLUE {BIG_MAX_CLUE}",
         table("s_big_clues", tiles.big_clues), ""]
    write_if_changed(OUT / "big_tiles.h", "\n".join(h))

    live = [p for p in pack["puzzles"] if not p.get("retired") and p["collection"] != "Murals"]
    mural_tiles = [p for p in pack["puzzles"]
                   if not p.get("retired") and p["collection"] == "Murals"]
    by_size = {s: sum(1 for p in live if p["width"] == s) for s in CLUE_ROOM}
    logo = "PICROSS"
    logo_rows = [".".join(LOGO_FONT[ch][y] for ch in logo) for y in range(5)]
    h = ["/* Generated by tools/gen_assets.py. Do not edit. */",
         "#ifndef PACK_H", "#define PACK_H",
         f"#define PUZZLE_COUNT {len(pack['puzzles'])}",
         f"#define COLLECTION_COUNT {len(pack['collections'])}",
         f"#define PACK_SAVE_SLOTS {SAVE_SLOTS}",
         f"#define STORY_PUZZLE_BASE {STORY_PUZZLE_BASE}",
         f"#define STORY_COLLECTION {STORY_COLLECTION}",
         f"#define PACK_LIVE_COUNT {len(live)}",
         f"#define PACK_COUNT_5 {by_size[5]}", f"#define PACK_COUNT_10 {by_size[10]}",
         f"#define PACK_COUNT_15 {by_size[15]}", f"#define PACK_COUNT_20 {by_size[20]}",
         f"#define MURAL_COUNT {len(pack['murals'])}",
         f"#define PACK_TILE_COUNT {len(mural_tiles)}",
         f"#define MURAL_COLLECTION {MURAL_COLLECTION}",
         f"#define PACK_TOTAL_POINTS {sum(points(p['width']) for p in live + mural_tiles)}",
         f"#define LOGO_W {len(logo_rows[0])}", "#define LOGO_H 5",
         "#define LOGO_ROWS \\"]
    h += [f'    "{r}"' + (" \\" if i < 4 else "") for i, r in enumerate(logo_rows)]
    h += ["#endif", ""]
    write_if_changed(OUT / "pack.h", "\n".join(h))

    c = ["/* Generated by tools/gen_assets.py: the pack for the host tests. */"]
    for name, data in bins.items():
        body = ",".join(str(b) for b in data)
        c.append(f"const unsigned char {name}_bin[{len(data)}] = {{{body}}};")
    write_if_changed(OUT / "pack_data.c", "\n".join(c) + "\n")
    print(f"assets: {len(pack['puzzles'])} puzzles, {tiles.count} BG1 tiles, "
          f"{sum(len(b) for b in bins.values())} bytes of pack data")


if __name__ == "__main__":
    main()
