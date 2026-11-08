#!/usr/bin/env python3
"""Imports the web game's story mode into data/story.json (ADR-010).

  python3 tools/import_story.py [--from PATH_TO_WEB_REPO] [--report OUT.md]

The web game (default ../../picross, the sibling checkout) is the source of
truth for the story and keeps growing: new chapters, jobs, fights and
scenes. This copies all of it into one committed file the ROM build reads,
so a web update reaches the cartridge by re-running `make import`.

It is also the compatibility gate. The SNES only knows the mechanics it has
code for (SUPPORTED below). Anything else the web data uses (a new enemy
power, a new gadget effect, a new scene feature, a character the font can't
draw) fails the import with a list of exactly what needs porting, and
writes nothing. `--report` writes that list as Markdown for the sync
workflow (.github/workflows/sync-web.yml) to file as an issue.

What it reads, all under src/:
- puzzles/chapters, runs, encounters, scenes, backdrops: *.json
- puzzles/run/*.json: puzzles only fights and digs use (pack puzzles come
  from data/pack.json, by id)
- puzzles/gadgets.ts: the gadget catalog
- ui/powers.ts: each enemy power's name and tip
- game/battle.ts, puzzles/runs.ts, puzzles/endless.ts, game/game.ts: the
  tuning constants (CONSTANTS below)

Save slots (ADR-003) are append-only, as for the pack: jobs and scenes keep
the slot they were first imported with, because the battery save records
job results and seen scenes by slot.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "data" / "story.json"
PACK = ROOT / "data" / "pack.json"

# What the SNES has code for. A field outside these sets fails the import
# as "needs porting", so a new web mechanic can't slip in unplayed.
SUPPORTED = {
    "chapter": {"id", "title", "order", "blurb", "jobs", "opening"},
    "run": {"id", "name", "hp", "encounters", "gadgets", "intro", "outro", "blurb",
            "order", "requires", "overtimeOf", "bossOf"},
    "encounter": {"id", "name", "kind", "puzzle", "blurb", "enemy", "boss", "scene", "rest"},
    "kind": {"fight", "dig", "rest"},
    "rest": {"seconds", "heal"},
    "enemy": {"name", "sprite", "hp", "attack", "fog", "scramble", "phases", "armor",
              "fakes", "suspects", "charge", "rotate"},
    "armor": {"weakPoints", "attack"},
    "phase": {"at", "name", "sprite", "attack", "taunt", "fog", "fogCount", "scramble",
              "scrambleCount", "swap", "ink", "inkCount", "charge", "rotate"},
    "lines": {"rows", "cols"},
    "scene": {"id", "backdrop", "cast", "start", "nodes"},
    "cast": {"name", "portrait"},
    "node": {"lines", "choices", "next"},
    "line": {"who", "text", "if"},
    "choice": {"text", "next", "set", "if"},
    "backdrop": {"id", "rows"},
    "puzzle": {"id", "name", "width", "height", "solution", "gadget"},
    "effect": {"blast", "shield", "wash"},
    "power": {"fog", "scramble", "armor", "fakes", "charge", "ink", "swap", "rotate"},
    # Board sizes with a layout on the SNES (docs/LAYOUT.md).
    "size": {5, 10, 15, 20},
}

# Tuning constants: (file, name). The SNES uses them as #defines, so a web
# rebalance flows through. A missing one fails the import.
CONSTANTS = [
    ("game/battle.ts", "CHIP_DAMAGE"),
    ("game/battle.ts", "WEAK_POINT_DAMAGE"),
    ("game/battle.ts", "ARMOR_HIT_LINES"),
    ("game/battle.ts", "ARMOR_HIT_DAMAGE"),
    ("game/battle.ts", "CALL_OUT_DAMAGE"),
    ("game/battle.ts", "FALSE_ACCUSATION_HEAL"),
    ("game/battle.ts", "ZAP_DAMAGE"),
    ("game/battle.ts", "WARM_UP_SHARE"),
    ("puzzles/runs.ts", "REST_LATE_HEAL"),
    ("puzzles/endless.ts", "ENDLESS_HP"),
    ("puzzles/endless.ts", "ENDLESS_HEAL"),
    ("puzzles/endless.ts", "ENDLESS_GADGETS"),
    ("puzzles/endless.ts", "BREAK_EVERY"),
    ("puzzles/endless.ts", "LOOKAHEAD"),
    ("game/game.ts", "MAX_HEARTS"),
    ("game/game.ts", "MAX_HINTS"),
]

# Characters outside printable ASCII. Emoji become <name> tokens that the
# ROM build draws with its own glyphs; typography becomes ASCII. Anything
# not listed fails the import, naming the character and where it is.
EMOJI = {
    "🎭": "mask", "🔍": "search", "⚙": "gear", "⚡": "bolt", "🔃": "turn",
    "🎯": "target", "👄": "lips", "🌫": "fog", "🦑": "squid", "💣": "bomb",
    "🩹": "patch", "🧽": "sponge", "☕": "cup", "👑": "crown", "👾": "invader",
}
TYPOGRAPHY = {
    "‘": "'", "’": "'", "“": '"', "”": '"', "—": "--",
    "–": "-", "…": "...", " ": " ", "×": "x",
}
VARIATION = {"️", "︎"}


class Problems:
    """Collects every problem before failing, so one run lists them all."""

    def __init__(self):
        self.port = []     # the web uses something the SNES has no code for
        self.invalid = []  # the data is broken or out of the SNES's limits

    def need_port(self, where, what):
        self.port.append(f"{where}: {what}")

    def bad(self, where, what):
        self.invalid.append(f"{where}: {what}")

    def __bool__(self):
        return bool(self.port or self.invalid)

    def markdown(self):
        out = []
        if self.port:
            out.append("### Needs porting to the SNES\n")
            out.append("The web game uses these, and the SNES has no code for them yet:\n")
            out += [f"- {p}" for p in self.port]
            out.append("")
        if self.invalid:
            out.append("### Invalid or out of the SNES's limits\n")
            out += [f"- {p}" for p in self.invalid]
            out.append("")
        return "\n".join(out)


# --- Reading values out of TypeScript ------------------------------------

def js_literal(src, pos=0):
    """Parses one JS literal (object, array, string, number, bool) at pos.

    Enough for the catalogs and constants this reads: unquoted keys, either
    quote style, trailing commas, // and /* */ comments. Returns (value, end).
    """
    def skip(i):
        while i < len(src):
            if src[i].isspace():
                i += 1
            elif src.startswith("//", i):
                i = src.index("\n", i) if "\n" in src[i:] else len(src)
            elif src.startswith("/*", i):
                i = src.index("*/", i) + 2
            else:
                break
        return i

    def value(i):
        i = skip(i)
        c = src[i]
        if c == "{":
            obj, i = {}, i + 1
            while True:
                i = skip(i)
                if src[i] == "}":
                    return obj, i + 1
                if src[i] in "'\"":
                    key, i = string(i)
                else:
                    m = re.compile(r"[A-Za-z_$][\w$]*").match(src, i)
                    if not m:
                        raise ValueError(f"unexpected {src[i:i + 20]!r}")
                    key, i = m.group(0), m.end()
                i = skip(i)
                if src[i] != ":":
                    raise ValueError(f"expected ':' near {src[i:i + 20]!r}")
                obj[key], i = value(i + 1)
                i = skip(i)
                if src[i] == ",":
                    i += 1
        if c == "[":
            arr, i = [], i + 1
            while True:
                i = skip(i)
                if src[i] == "]":
                    return arr, i + 1
                item, i = value(i)
                arr.append(item)
                i = skip(i)
                if src[i] == ",":
                    i += 1
        if c in "'\"":
            return string(i)
        m = re.compile(r"-?\d+(\.\d+)?").match(src, i)
        if m:
            text = m.group(0)
            return (float(text) if "." in text else int(text)), m.end()
        for word, val in (("true", True), ("false", False), ("null", None)):
            if src.startswith(word, i):
                return val, i + len(word)
        raise ValueError(f"unsupported literal near {src[i:i + 30]!r}")

    def string(i):
        quote, i, out = src[i], i + 1, []
        while src[i] != quote:
            if src[i] == "\\":
                nxt = src[i + 1]
                out.append({"n": "\n", "t": "\t"}.get(nxt, nxt))
                i += 2
            else:
                out.append(src[i])
                i += 1
        return "".join(out), i + 1

    return value(pos)


def ts_value(path, name, problems):
    """The literal assigned to `export const NAME` (or `const NAME`) in a .ts file."""
    text = path.read_text(encoding="utf-8")
    m = re.search(rf"(?:export\s+)?const\s+{name}\b[^=]*=\s*", text)
    if not m:
        problems.need_port(path.name, f"`{name}` is gone; the importer reads it (CONSTANTS)")
        return None
    try:
        return js_literal(text, m.end())[0]
    except (ValueError, IndexError) as e:
        problems.need_port(path.name, f"`{name}` isn't a plain literal any more ({e})")
        return None


# --- Text -------------------------------------------------------------------

def clean_text(text, where, problems):
    """Plain ASCII plus <token>s for the glyphs the ROM build draws."""
    out = []
    for ch in text:
        if ch in VARIATION:
            continue
        if ch in EMOJI:
            out.append(f"<{EMOJI[ch]}>")
        elif ch in TYPOGRAPHY:
            out.append(TYPOGRAPHY[ch])
        elif ch == "\n" or (" " <= ch <= "~" and ch not in "<>"):
            out.append(ch)
        else:
            problems.need_port(where, f"character {ch!r} (U+{ord(ch):04X}) has no SNES glyph "
                               "(add it to EMOJI or TYPOGRAPHY in tools/import_story.py)")
    return "".join(out)


# --- Validation ---------------------------------------------------------------

def check_fields(obj, kind, where, problems):
    if not isinstance(obj, dict):
        problems.bad(where, f"expected an object for {kind}")
        return False
    for key in obj:
        if key not in SUPPORTED[kind]:
            problems.need_port(where, f"{kind} field `{key}`")
    return True


def check_lines(data, where, width, height, problems):
    if data is None:
        return None
    if not check_fields(data, "lines", where, problems):
        return None
    rows, cols = data.get("rows", []), data.get("cols", [])
    for name, lst, size in (("rows", rows, height), ("cols", cols, width)):
        if not all(isinstance(i, int) and 0 <= i < size for i in lst):
            problems.bad(where, f"{name} {lst} out of range for a {width}x{height} board")
    return {"rows": rows, "cols": cols}


def load_dir(web, sub):
    out = {}
    for path in sorted((web / "src/puzzles" / sub).glob("*.json")):
        out[path.stem] = (path, json.loads(path.read_text(encoding="utf-8")))
    return out


def slots_for(previous, key, ids):
    """Append-only slot list: old ids keep their place, new ids go on the end."""
    old = list(previous.get(key, []))
    for i in ids:
        if i not in old:
            old.append(i)
    return old


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--from", dest="src", default=str(ROOT.parent.parent / "picross"))
    parser.add_argument("--report", help="write the problems as Markdown here on failure")
    args = parser.parse_args()
    web = Path(args.src)
    src = web / "src"
    problems = Problems()

    pack = json.loads(PACK.read_text(encoding="utf-8"))
    pack_ids = {p["id"]: p for p in pack["puzzles"] if not p.get("retired")}

    # Puzzles only the story uses.
    story_puzzles = {}
    for pid, (path, p) in load_dir(web, "run").items():
        where = f"run/{path.name}"
        check_fields(p, "puzzle", where, problems)
        story_puzzles[p["id"]] = {
            "id": p["id"], "name": clean_text(p["name"], where, problems),
            "width": p["width"], "height": p["height"], "rows": p["solution"],
            **({"gadget": p["gadget"]} if "gadget" in p else {}),
        }
    pack_gadgets = {}
    for path in sorted((src / "puzzles/pack").glob("*.json")):
        p = json.loads(path.read_text(encoding="utf-8"))
        if "gadget" in p:
            pack_gadgets[p["id"]] = p["gadget"]

    def puzzle_size(pid):
        if pid in story_puzzles:
            p = story_puzzles[pid]
        elif pid in pack_ids:
            p = pack_ids[pid]
        else:
            return None
        return p["width"], p["height"]

    # Gadgets: the catalog in gadgets.ts.
    gadgets = []
    catalog = ts_value(src / "puzzles/gadgets.ts", "catalog", problems) or []
    for g in catalog:
        where = f"gadgets.ts {g.get('id')}"
        kind = (g.get("effect") or {}).get("kind")
        if kind not in SUPPORTED["effect"]:
            problems.need_port(where, f"gadget effect `{kind}`")
        gadgets.append({
            "id": g["id"], "name": clean_text(g["name"], where, problems),
            "description": clean_text(g["description"], where, problems),
            "effect": g["effect"],
        })
    gadget_ids = {g["id"] for g in gadgets}

    # Power tips.
    tips = {}
    powers = ts_value(src / "ui/powers.ts", "POWERS", problems) or {}
    for key, p in powers.items():
        where = f"powers.ts {key}"
        if key not in SUPPORTED["power"]:
            problems.need_port(where, f"enemy power `{key}`")
        tips[key] = {"name": clean_text(p["name"], where, problems),
                     "tip": clean_text(p["tip"], where, problems)}
    for key in SUPPORTED["power"] - set(tips):
        problems.bad("powers.ts", f"no tip for `{key}`")

    constants = {}
    for rel, name in CONSTANTS:
        constants[name] = ts_value(src / rel, name, problems)

    # Backdrops.
    backdrops = {}
    for bid, (path, b) in load_dir(web, "backdrops").items():
        where = f"backdrops/{path.name}"
        check_fields(b, "backdrop", where, problems)
        rows = b.get("rows", [])
        if not rows or any(len(r) != len(rows[0]) or set(r) - {"#", "."} for r in rows):
            problems.bad(where, "rows must be equal-length strings of # and .")
        backdrops[b["id"]] = rows

    # Scenes.
    scenes = {}
    flags = ["won"]  # bit 0: the outcome a job's outro reads
    for sid, (path, s) in load_dir(web, "scenes").items():
        where = f"scenes/{path.name}"
        if not check_fields(s, "scene", where, problems):
            continue
        if s.get("backdrop") is not None and s["backdrop"] not in backdrops:
            problems.bad(where, f"unknown backdrop {s['backdrop']}")
        cast = {}
        for who, member in s.get("cast", {}).items():
            check_fields(member, "cast", f"{where} cast {who}", problems)
            cast[who] = {"name": clean_text(member["name"], where, problems),
                         "portrait": member["portrait"]}
        nodes = {}
        for nid, node in s["nodes"].items():
            nwhere = f"{where} node {nid}"
            check_fields(node, "node", nwhere, problems)
            out = {}
            lines = []
            for line in node.get("lines", []):
                check_fields(line, "line", nwhere, problems)
                if line.get("who") is not None and line["who"] not in cast:
                    problems.bad(nwhere, f"unknown speaker {line['who']}")
                lines.append({k: (clean_text(v, nwhere, problems) if k == "text" else v)
                              for k, v in line.items()})
            if lines:
                out["lines"] = lines
            choices = []
            for choice in node.get("choices", []):
                check_fields(choice, "choice", nwhere, problems)
                for flag in choice.get("set", {}):
                    if flag not in flags:
                        flags.append(flag)
                choices.append({k: (clean_text(v, nwhere, problems) if k == "text" else v)
                                for k, v in choice.items()})
            if choices:
                out["choices"] = choices
            if "next" in node:
                out["next"] = node["next"]
            for item in lines + choices:
                cond = item.get("if")
                if cond is not None and cond.lstrip("!") not in flags:
                    flags.append(cond.lstrip("!"))
            nodes[nid] = out
        start = s.get("start", "start")
        for nid, node in nodes.items():
            targets = [node.get("next")] + [c.get("next") for c in node.get("choices", [])]
            for t in targets:
                if t is not None and t not in nodes:
                    problems.bad(where, f"node {nid} goes to unknown node {t}")
        if start not in nodes:
            problems.bad(where, f"start node {start} doesn't exist")
        scenes[s["id"]] = {"id": s["id"], "backdrop": s.get("backdrop"), "cast": cast,
                           "start": start, "nodes": nodes}
    if len(flags) > 8:
        problems.bad("scenes", f"{len(flags)} scene flags; the SNES keeps 8 ({flags})")

    # Encounters.
    encounters = {}
    for eid, (path, e) in load_dir(web, "encounters").items():
        where = f"encounters/{path.name}"
        if not check_fields(e, "encounter", where, problems):
            continue
        if e.get("kind") not in SUPPORTED["kind"]:
            problems.need_port(where, f"encounter kind `{e.get('kind')}`")
            continue
        size = puzzle_size(e.get("puzzle"))
        if size is None:
            problems.bad(where, f"unknown puzzle {e.get('puzzle')}")
            continue
        w, h = size
        if w != h or w not in SUPPORTED["size"]:
            problems.bad(where, f"a {w}x{h} board has no SNES layout")
        if e.get("scene") is not None and e["scene"] not in scenes:
            problems.bad(where, f"unknown scene {e['scene']}")
        out = {"id": e["id"], "name": clean_text(e["name"], where, problems),
               "kind": e["kind"], "puzzle": e["puzzle"]}
        if e.get("blurb"):
            out["blurb"] = clean_text(e["blurb"], where, problems)
        if e.get("boss"):
            out["boss"] = True
        if e.get("scene"):
            out["scene"] = e["scene"]
        if e["kind"] == "rest":
            check_fields(e.get("rest", {}), "rest", where, problems)
            out["rest"] = e.get("rest")
        if e["kind"] == "fight":
            out["enemy"] = import_enemy(e.get("enemy"), where, w, h, problems)
        encounters[e["id"]] = out

    # Runs (jobs, overtime, boss fights).
    runs = {}
    for rid, (path, r) in load_dir(web, "runs").items():
        where = f"runs/{path.name}"
        if not check_fields(r, "run", where, problems):
            continue
        for enc in r.get("encounters", []):
            if enc not in encounters:
                problems.bad(where, f"unknown encounter {enc}")
        for g in r.get("gadgets", []):
            if g not in gadget_ids:
                problems.bad(where, f"unknown gadget {g}")
        for key in ("intro", "outro"):
            if r.get(key) is not None and r[key] not in scenes:
                problems.bad(where, f"unknown {key} scene {r[key]}")
        out = {k: v for k, v in r.items()}
        out["name"] = clean_text(r["name"], where, problems)
        if "blurb" in r:
            out["blurb"] = clean_text(r["blurb"], where, problems)
        runs[r["id"]] = out
    for r in runs.values():
        for key in ("requires", "overtimeOf", "bossOf"):
            if r.get(key) is not None and r[key] not in runs:
                problems.bad(f"runs/{r['id']}.json", f"{key} names unknown job {r[key]}")

    # Chapters.
    chapters = []
    in_chapter = {}
    for cid, (path, c) in load_dir(web, "chapters").items():
        where = f"chapters/{path.name}"
        if not check_fields(c, "chapter", where, problems):
            continue
        for job in c.get("jobs", []):
            if job not in runs:
                problems.bad(where, f"unknown job {job}")
            in_chapter[job] = c["id"]
        if c.get("opening") is not None and c["opening"] not in scenes:
            problems.bad(where, f"unknown opening scene {c['opening']}")
        chapters.append({"id": c["id"], "title": clean_text(c["title"], where, problems),
                         "order": c["order"], "blurb": clean_text(c.get("blurb", ""), where, problems),
                         "jobs": c["jobs"], **({"opening": c["opening"]} if c.get("opening") else {})})
    chapters.sort(key=lambda c: c["order"])

    # Gadgets named by puzzles must exist.
    for pid, g in list(pack_gadgets.items()) + [(p["id"], p.get("gadget")) for p in story_puzzles.values()]:
        if g is not None and g not in gadget_ids:
            problems.bad(f"puzzle {pid}", f"unknown gadget {g}")

    # Art the story names: the art import must have it (data/art/), or the
    # ROM build would have nothing to draw.
    art = ROOT / "data" / "art"
    if art.exists():
        for s in scenes.values():
            for who, m in s["cast"].items():
                if not (art / "cast" / f"{m['portrait']}.txt").exists():
                    problems.bad(f"scenes/{s['id']}.json", f"no portrait art for {m['portrait']} "
                                 "(run tools/import_art.py, or draw one)")

    # Enemy sprites without art (an emoji placeholder on the web) draw as the
    # web's own fallback, enemies/unknown. A warning, not a failure: the
    # fight still plays.
    if art.exists():
        for e in encounters.values():
            enemy = e.get("enemy")
            if not enemy:
                continue
            for form in [enemy] + enemy.get("phases", []):
                sprite = form.get("sprite")
                if sprite is not None and not (art / "enemies" / f"{sprite}.txt").exists():
                    print(f"warning: encounters/{e['id']}.json: no art for sprite {sprite!r}; "
                          "it draws as enemies/unknown", file=sys.stderr)
                    form["sprite"] = "unknown"

    if problems:
        print(problems.markdown(), file=sys.stderr)
        if args.report:
            Path(args.report).write_text(problems.markdown(), encoding="utf-8")
        print("data/story.json not written: fix the web data or port the features above.",
              file=sys.stderr)
        sys.exit(1)

    previous = json.loads(OUT.read_text(encoding="utf-8")) if OUT.exists() else {}
    job_slots = slots_for(previous, "jobSlots", sorted(runs))
    scene_slots = slots_for(previous, "sceneSlots", sorted(scenes))

    story = {
        "source": "picross web game, src/puzzles (tools/import_story.py)",
        "jobSlots": job_slots,
        "sceneSlots": scene_slots,
        "flags": flags,
        "constants": constants,
        "gadgets": gadgets,
        "tips": tips,
        "chapters": chapters,
        "runs": [runs[i] for i in sorted(runs)],
        "encounters": [encounters[i] for i in sorted(encounters)],
        "puzzles": [story_puzzles[i] for i in sorted(story_puzzles)],
        "packGadgets": pack_gadgets,
        "scenes": [scenes[i] for i in sorted(scenes)],
        "backdrops": backdrops,
    }
    OUT.write_text(json.dumps(story, indent=1, ensure_ascii=True) + "\n", encoding="utf-8")
    print(f"{OUT.relative_to(ROOT)}: {len(chapters)} chapters, {len(runs)} runs, "
          f"{len(encounters)} encounters, {len(scenes)} scenes, {len(story_puzzles)} story puzzles")


def import_enemy(e, where, w, h, problems):
    if not check_fields(e, "enemy", where, problems):
        return None
    out = {"name": clean_text(e["name"], where, problems), "sprite": e.get("sprite"),
           "hp": e["hp"], "attack": e["attack"]}
    if not (1 <= e["hp"] <= 255):
        problems.bad(where, f"enemy hp {e['hp']}: the SNES keeps 1-255")
    for key in ("fog", "scramble", "fakes", "suspects"):
        if key in e:
            out[key] = check_lines(e[key], f"{where} {key}", w, h, problems)
    for key in ("charge", "rotate"):
        if key in e:
            out[key] = e[key]
    if "armor" in e:
        check_fields(e["armor"], "armor", where, problems)
        out["armor"] = {"weakPoints": check_lines(e["armor"].get("weakPoints"),
                                                  f"{where} armor", w, h, problems),
                        "attack": e["armor"].get("attack", 0)}
    phases = []
    for i, p in enumerate(e.get("phases", [])):
        pwhere = f"{where} phase {i + 1}"
        check_fields(p, "phase", pwhere, problems)
        q = {k: v for k, v in p.items()}
        for key in ("fog", "scramble"):
            if key in p:
                q[key] = check_lines(p[key], pwhere, w, h, problems)
        for key in ("name", "taunt"):
            if key in p:
                q[key] = clean_text(p[key], pwhere, problems)
        phases.append(q)
    if phases:
        out["phases"] = phases
    if len(phases) > 8:
        problems.bad(where, f"{len(phases)} phases; the SNES keeps 8")
    return out


if __name__ == "__main__":
    main()
