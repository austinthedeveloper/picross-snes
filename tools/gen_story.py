#!/usr/bin/env python3
"""Turns data/story.json into ROM tables (ADR-010). Called by gen_assets.py.

Writes into assets/generated/:
  story_const.h    the web's tuning constants, counts, flag and glyph codes
  story_tab.bin    chapters, runs, encounters, enemies, scenes, gadgets
  story_str.bin    the string index: 3 bytes a string (text bank, u16 offset)
  story_txtN.bin   the strings, NUL-terminated, at most 32 KiB a bank
  story_data.asm   the .incbin sections for data.asm
  story_data.c     the same bytes as C arrays, for the host tests

story_tab.bin layout (all offsets are u16 within story_tab.bin, strings are
u16 string ids, 0xFFFF for none; one-byte references use 0xFF for none):

  header      u16 count + u16 offset of the index for each of, in order:
              chapters, runs, encounters, scenes, gadgets, backdrops
  index       u16 record offset per item
  chapter     order, opening scene, u16 title, u16 blurb, n, job runs[n]
  run         slot, hp, order, requires, overtimeOf, bossOf, intro, outro,
              u16 name, u16 blurb, n, encounters[n], g, gadgets[g]
  encounter   kind, boss, scene, rest seconds, rest heal, gadget,
              u16 puzzle (ROM puzzle index), u16 name, u16 blurb, has enemy,
              0; a fight's enemy follows at +14:
  enemy       sprite, hp, attack, charge, rotate, armor attack, has armor,
              phases, u16 name, u16 fog, scramble, weak, fakes, suspects
              (line lists: count, ids), then per phase:
              at_hp, attack, sprite, fog count, scramble count, swap, ink,
              ink count, charge, rotate, u16 name, u16 taunt, u16 fog,
              u16 scramble
  scene       backdrop, cast n, (portrait, u16 name)[n], start node, nodes m,
              u16 node offsets[m]
  node        next, lines n, (who, cond, u16 text)[n], choices k,
              (next, cond, set mask, set values, u16 text)[k]
  gadget      effect, value, icon, u16 name, u16 description
  backdrop    width, height, rows as bits (MSB first, whole bytes a row)

A line id is a row index, or 20 + a column index (battle.h). A condition
byte is 0 for none, else flag index + 1, with bit 7 set for "not".
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STORY = ROOT / "data" / "story.json"
ART = ROOT / "data" / "art"

LINE_COL = 20
NONE8 = 0xFF
NONE16 = 0xFFFF
BANK = 0x8000
KINDS = {"fight": 0, "dig": 1, "rest": 2}
EFFECTS = {"blast": 0, "shield": 1, "wash": 2}

# Glyph tokens in story text (tools/import_story.py EMOJI) and the byte each
# draws as. The font has these glyphs at the same codes (font.py).
GLYPHS = ["mask", "search", "gear", "bolt", "turn", "target", "lips", "fog",
          "squid", "bomb", "patch", "sponge", "cup", "crown", "invader"]
GLYPH_BASE = 0x80


def u16(n):
    assert 0 <= n <= 0xFFFF, n
    return bytes((n & 0xFF, n >> 8))


class Strings:
    def __init__(self):
        self.ids = {}
        self.banks = [bytearray()]
        self.index = bytearray()

    def add(self, text):
        if text is None:
            return NONE16
        if text in self.ids:
            return self.ids[text]
        data = encode_text(text) + b"\0"
        if len(self.banks[-1]) + len(data) > BANK:
            self.banks.append(bytearray())
        bank = len(self.banks) - 1
        self.index += bytes((bank,)) + u16(len(self.banks[bank]))
        self.banks[bank] += data
        self.ids[text] = len(self.ids)
        return self.ids[text]


def encode_text(text):
    out = bytearray()
    i = 0
    while i < len(text):
        if text[i] == "<":
            end = text.index(">", i)
            out.append(GLYPH_BASE + GLYPHS.index(text[i + 1:end]))
            i = end + 1
        else:
            out.append(ord(text[i]))
            i += 1
    return bytes(out)


def line_ids(lines):
    """A {rows, cols} set as one ordered list: rows, then columns (web lineKeys)."""
    if lines is None:
        return None
    return list(lines.get("rows", [])) + [LINE_COL + c for c in lines.get("cols", [])]


def at_hp(at, hp):
    # The web starts a phase once enemyHp <= at * hp, in doubles. For whole
    # HP that's enemyHp <= floor(at * hp); Python's float is the same double.
    import math
    return math.floor(at * hp)


def art_names(group):
    return sorted(p.stem for p in (ART / group).glob("*.txt"))


def build(pack, story_puzzle_base):
    story = json.loads(STORY.read_text(encoding="utf-8"))
    strings = Strings()
    tab = bytearray(24)  # header, filled in at the end
    lists = {}

    def add_list(ids):
        if ids is None:
            return 0
        key = tuple(ids)
        if key not in lists:
            lists[key] = len(tab)
            tab.extend(bytes((len(ids),)) + bytes(ids))
        return lists[key]

    sprites = art_names("enemies")
    portraits = art_names("cast")
    gadget_icons = art_names("gadgets")
    pack_slot = {p["id"]: p["slot"] for p in pack["puzzles"]}
    story_puzzles = {p["id"]: i for i, p in enumerate(story["puzzles"])}
    STORY_PUZZLE_BASE = story_puzzle_base

    def puzzle_index(pid):
        if pid in story_puzzles:
            return STORY_PUZZLE_BASE + story_puzzles[pid]
        return pack_slot[pid]

    scenes = story["scenes"]
    scene_ix = {s["id"]: i for i, s in enumerate(scenes)}
    runs = story["runs"]
    run_ix = {r["id"]: i for i, r in enumerate(runs)}
    encs = story["encounters"]
    enc_ix = {e["id"]: i for i, e in enumerate(encs)}
    gadgets = story["gadgets"]
    gadget_ix = {g["id"]: i for i, g in enumerate(gadgets)}
    backdrops = sorted(story["backdrops"])
    backdrop_ix = {b: i for i, b in enumerate(backdrops)}
    flags = story["flags"]
    puzzle_gadget = dict(story["packGadgets"])
    for p in story["puzzles"]:
        if "gadget" in p:
            puzzle_gadget[p["id"]] = p["gadget"]

    def sc(sid):
        return NONE8 if sid is None else scene_ix[sid]

    def rn(rid):
        return NONE8 if rid is None else run_ix[rid]

    def cond(c):
        if not c:
            return 0
        neg = c.startswith("!")
        return (flags.index(c.lstrip("!")) + 1) | (0x80 if neg else 0)

    def section(items, record):
        index_at = len(tab)
        tab.extend(bytes(2 * len(items)))
        for i, item in enumerate(items):
            data = record(item)  # may place line lists in tab first
            tab[index_at + 2 * i:index_at + 2 * i + 2] = u16(len(tab))
            tab.extend(data)
        return len(items), index_at

    def chapter(c):
        return (bytes((c["order"], sc(c.get("opening"))))
                + u16(strings.add(c["title"])) + u16(strings.add(c.get("blurb")))
                + bytes((len(c["jobs"]),)) + bytes(run_ix[j] for j in c["jobs"]))

    def run(r):
        slot = story["jobSlots"].index(r["id"])
        assert slot < 256 and r["hp"] < 256
        return (bytes((slot, r["hp"], r.get("order", 99), rn(r.get("requires")),
                       rn(r.get("overtimeOf")), rn(r.get("bossOf")),
                       sc(r.get("intro")), sc(r.get("outro"))))
                + u16(strings.add(r["name"])) + u16(strings.add(r.get("blurb")))
                + bytes((len(r["encounters"]),)) + bytes(enc_ix[e] for e in r["encounters"])
                + bytes((len(r.get("gadgets", [])),))
                + bytes(gadget_ix[g] for g in r.get("gadgets", [])))

    def enemy(e):
        def sprite(name):
            return NONE8 if name is None else sprites.index(name)
        out = bytearray(bytes((
            sprite(e["sprite"]), e["hp"], e["attack"], e.get("charge", 0), e.get("rotate", 0),
            e["armor"]["attack"] if "armor" in e else 0, 1 if "armor" in e else 0,
            len(e.get("phases", [])))))
        out += u16(strings.add(e["name"]))
        for key in ("fog", "scramble"):
            out += u16(add_list(line_ids(e.get(key))))
        out += u16(add_list(line_ids(e["armor"]["weakPoints"]) if "armor" in e else None))
        for key in ("fakes", "suspects"):
            out += u16(add_list(line_ids(e.get(key))))
        for p in e.get("phases", []):
            out += bytes((at_hp(p["at"], e["hp"]), p.get("attack", NONE8),
                          sprite(p.get("sprite")), p.get("fogCount", NONE8),
                          p.get("scrambleCount", NONE8), 1 if p.get("swap") else 0,
                          p.get("ink", 0), p.get("inkCount", 1), p.get("charge", 0),
                          p.get("rotate", 0)))
            out += u16(strings.add(p.get("name"))) + u16(strings.add(p.get("taunt")))
            out += u16(add_list(line_ids(p.get("fog"))))
            out += u16(add_list(line_ids(p.get("scramble"))))
        return bytes(out)

    def encounter_record(e):
        rest = e.get("rest") or {}
        assert rest.get("seconds", 0) < 256
        gadget = puzzle_gadget.get(e["puzzle"])
        body = enemy(e["enemy"]) if e.get("enemy") else b""
        rec = (bytes((KINDS[e["kind"]], 1 if e.get("boss") else 0, sc(e.get("scene")),
                      rest.get("seconds", 0), rest.get("heal", 0),
                      NONE8 if gadget is None else gadget_ix[gadget]))
               + u16(puzzle_index(e["puzzle"])) + u16(strings.add(e["name"]))
               + u16(strings.add(e.get("blurb"))) + bytes((1 if body else 0, 0)))
        assert len(rec) == 14
        return rec + body

    def gadget(g):
        eff = g["effect"]
        value = eff.get("damage", eff.get("hits", 0))
        icon = gadget_icons.index(g["id"]) if g["id"] in gadget_icons else NONE8
        return (bytes((EFFECTS[eff["kind"]], value, icon))
                + u16(strings.add(g["name"])) + u16(strings.add(g["description"])))

    def scene(s):
        cast = list(s["cast"])
        out = bytearray(bytes((NONE8 if s.get("backdrop") is None else backdrop_ix[s["backdrop"]],
                               len(cast))))
        for who in cast:
            m = s["cast"][who]
            out += bytes((portraits.index(m["portrait"]),)) + u16(strings.add(m["name"]))
        nodes = list(s["nodes"])
        out += bytes((nodes.index(s["start"]), len(nodes)))
        index_at = len(out)
        out += bytes(2 * len(nodes))
        for i, nid in enumerate(nodes):
            n = s["nodes"][nid]
            # Node offsets are relative to the scene record.
            out[index_at + 2 * i:index_at + 2 * i + 2] = u16(len(out))
            out += bytes((NONE8 if n.get("next") is None else nodes.index(n["next"]),
                          len(n.get("lines", []))))
            for line in n.get("lines", []):
                who = line.get("who")
                out += bytes((NONE8 if who is None else cast.index(who), cond(line.get("if"))))
                out += u16(strings.add(line["text"]))
            out += bytes((len(n.get("choices", [])),))
            for ch in n.get("choices", []):
                sets = ch.get("set", {})
                mask = sum(1 << flags.index(f) for f in sets)
                vals = sum(1 << flags.index(f) for f, v in sets.items() if v)
                out += bytes((NONE8 if ch.get("next") is None else nodes.index(ch["next"]),
                              cond(ch.get("if")), mask, vals))
                out += u16(strings.add(ch["text"]))
        return bytes(out)

    def backdrop(bid):
        rows = story["backdrops"][bid]
        w, h = len(rows[0]), len(rows)
        out = bytearray((w, h))
        for row in rows:
            v = 0
            nbytes = (w + 7) // 8
            for x, ch in enumerate(row):
                if ch == "#":
                    v |= 1 << (nbytes * 8 - 1 - x)
            out += v.to_bytes(nbytes, "big")
        return bytes(out)

    header = []
    header.append(section(story["chapters"], chapter))
    header.append(section(runs, run))
    header.append(section(encs, encounter_record))
    header.append(section(scenes, scene))
    header.append(section(gadgets, gadget))
    header.append(section(backdrops, backdrop))
    for i, (count, at) in enumerate(header):
        tab[4 * i:4 * i + 4] = u16(count) + u16(at)
    assert len(tab) <= BANK, f"story_tab.bin is {len(tab)} bytes; split it"

    c = story["constants"]
    share = c["WARM_UP_SHARE"]
    div = round(1 / share)
    assert abs(1 / div - share) < 1e-9, "WARM_UP_SHARE must be 1/k"
    h = ["/* Generated by tools/gen_story.py from data/story.json. Do not edit. */",
         "#ifndef STORY_CONST_H", "#define STORY_CONST_H"]
    for name in ("CHIP_DAMAGE", "WEAK_POINT_DAMAGE", "ARMOR_HIT_LINES", "ARMOR_HIT_DAMAGE",
                 "CALL_OUT_DAMAGE", "FALSE_ACCUSATION_HEAL", "ZAP_DAMAGE", "REST_LATE_HEAL",
                 "ENDLESS_HP", "ENDLESS_HEAL", "BREAK_EVERY", "LOOKAHEAD"):
        h.append(f"#define {name} {c[name]}")
    h.append(f"#define WARM_UP_DIV {div}")
    # The web's --backdrop token (src/styles/tokens.css): scenery behind a scene.
    r, g, b = 0xE2, 0xDE, 0xD3
    h.append(f"#define COLOUR_BACKDROP 0x{(r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10):04X}")
    h += [f"#define STORY_CHAPTERS {len(story['chapters'])}",
          f"#define STORY_RUNS {len(runs)}",
          f"#define STORY_ENCOUNTERS {len(encs)}",
          f"#define STORY_SCENES {len(scenes)}",
          f"#define STORY_GADGETS {len(gadgets)}",
          f"#define STORY_BACKDROPS {len(backdrops)}",
          f"#define STORY_SCENE_SLOTS {len(story['sceneSlots'])}",
          f"#define STORY_JOB_SLOTS {len(story['jobSlots'])}",
          f"#define STORY_TEXT_BANKS {len(strings.banks)}",
          f"#define STORY_SPRITES {len(sprites)}",
          f"#define STORY_PORTRAITS {len(portraits)}"]
    for i, f in enumerate(flags):
        h.append(f"#define FLAG_{f.upper().replace('-', '_')} {i}")
    for i, g in enumerate(GLYPHS):
        h.append(f"#define GLYPH_{g.upper()} 0x{GLYPH_BASE + i:02X}")
    for g in gadgets:
        h.append(f"#define GADGET_{g['id'].upper().replace('-', '_')} {gadget_ix[g['id']]}")
    h.append("#define SPRITE_UNKNOWN %d" % sprites.index("unknown"))
    cname = lambda s: s.upper().replace("-", "_")
    h += [f"#define SCENE_ID_{cname(s['id'])} {i}" for i, s in enumerate(scenes)]
    h += [f"#define RUN_ID_{cname(r['id'])} {i}" for i, r in enumerate(runs)]
    h += ["#endif", ""]

    bins = {"story_tab": bytes(tab), "story_str": bytes(strings.index)}
    for i, b in enumerate(strings.banks):
        bins[f"story_txt{i}"] = bytes(b)
    return bins, "\n".join(h), story


def story_puzzle_bins(story):
    """(width, height, rows) for each story puzzle, in story order."""
    return [(p["width"], p["height"], p["rows"], p["name"]) for p in story["puzzles"]]


def asm_and_c(bins):
    asm = ["; Generated by tools/gen_story.py. Do not edit."]
    for name in bins:
        asm += [f'.section ".rodata_{name}" superfree', f"{name}_bin:",
                f'.incbin "assets/generated/{name}.bin"', ".ends", ""]
    c = ["/* Generated by tools/gen_story.py: the story bins for the host tests. */"]
    for name, data in bins.items():
        body = ",".join(str(b) for b in data) or "0"
        c.append(f"const unsigned char {name}_bin[] = {{{body}}};")
    banks = [n for n in bins if n.startswith("story_txt")]
    h = ["/* Generated by tools/gen_story.py. Include once (story.c). */"]
    h += [f"ROM_DECLARE({n});" for n in bins]
    h.append("#define STORY_TEXT_BANK_INIT(a) do { "
             + " ".join(f"(a)[{i}] = ROM_DATA({n});" for i, n in enumerate(banks)) + " } while (0)")
    return "\n".join(asm) + "\n", "\n".join(c) + "\n", "\n".join(h) + "\n"
