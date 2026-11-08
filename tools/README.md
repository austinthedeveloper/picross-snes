# Tools

Everything here is plain Python 3 (standard library only; Pillow isn't
needed) plus two Lua scripts for MesenCE. Each script's docstring has the
details: `python3 tools/<name>.py --help`.

## Importing from the web game (ADR-010, ADR-011)

`make import` runs the three importers, and the daily sync workflow runs
the same thing (`WEB=` picks the checkout).

| Script | Does |
| --- | --- |
| `import_pack.py` | The free-play pack and murals into `data/pack.json` (append-only slots) |
| `import_art.py` | Portraits, enemy sprites, icons and glyphs into `data/art/` (`--check`, `--preview`) |
| `import_story.py` | The story into `data/story.json`; fails, listing what to port, on anything the SNES can't do (`--report`) |
| `sync_summary.py` | What an import changed, in words, for the sync PR |

## Building the ROM's data

`make assets` (and every ROM build) runs `gen_assets.py`, which calls the
others.

| Script | Does |
| --- | --- |
| `gen_assets.py` | Font, board tiles, palettes, the pack tables, sound (`--check` validates) |
| `gen_story.py` | Story tables, text banks, `story_const.h` |
| `gen_art.py` | 4bpp tiles and palettes from `data/art/` |
| `font.py` | The 5x7 font, lower case, story glyphs, narrow digits |
| `sfx.py` | The synthesized sound effects (an IT module) |
| `png.py` | Reads and writes PNGs |

## Testing

| Script | Does |
| --- | --- |
| `smoke.py` | Runs timelines headless with contact sheets: `NAME ...`, `--all`, `--list`, or on a CI or release ROM (`--run ID`, `--release TAG`, `--rom PATH`). `make smoke` / `make smoke-all` call it |
| `board_pokes.py` | Writes a `.pokes` file that leaves a pack or story board a few cells short of solved, and the d-pad presses to finish it |
| `pokes.py` | Turns a `.pokes` file into the Mesen driver's pokes |
| `contact_sheet.py` | Tiles screenshots into one PNG |
| `mesen_smoke.lua` | The MesenCE driver: timeline input, screenshots, probes, pokes |
| `mesen_voices.lua` | Prints each SPC700 voice start (sound checks) |
| `check_tcc_pitfalls.py` | C that 816-tcc miscompiles (`make check`) |
| `find_unused.py` | Header functions nothing calls (`make check`, strict) |

## GitHub

| Script | Does |
| --- | --- |
| `pr_stack.py` | Shows stacked PRs; `merge TOP` merges a stack bottom-first, retargeting each to `main`; `strays` finds merged PRs that never reached `main` |
| `release.py` | `VERSION` starts a release branch (version bump, drafted notes); `VERSION --check` says when main is ready to tag |
