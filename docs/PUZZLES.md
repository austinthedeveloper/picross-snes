# Puzzles

## Where they come from

The web game is the source of truth. `make import` runs
`tools/import_pack.py`, which reads `../../picross/src/puzzles/`:

- `pack/*.json`: id, name, size, rows of `#` and `.`
- `index.ts` `ORDER`: the play order, with a comment line before each
  collection (`// Dungeon crawl`)
- `ratings.json`: line-solver passes, for the difficulty band
- `murals/*.json`: big pictures, split into 15x15 tile puzzles named
  after the mural (ADR-008), in collection "Murals"

and writes `data/pack.json` (committed). The web game's tests already prove
every puzzle has one solution reachable without guessing; this repo
doesn't re-check that.

## Slots are forever

The battery save keeps a record per puzzle, indexed by **slot**. A slot is
the puzzle's position in `data/pack.json`, and it never changes:

- New puzzles are appended with new slots, whatever their collection.
- A puzzle removed from the web pack stays, marked `"retired": true`, so
  old saves still line up (the game will skip retired slots when that
  happens; none are retired yet).
- A changed picture prints a warning: a published picture should be
  frozen; give a changed drawing a new id instead.

Slots 0-522 are the collection puzzles and 523-612 the mural tiles (each
mural's tiles consecutive, row by row). The save has room for 700 slots (`SAVE_SLOTS`, ADR-003). Past that the save
layout changes.

## ROM format

`tools/gen_assets.py` writes these into `assets/generated/`, and
`data.asm` includes them:

- `pack_info.bin`, 8 bytes a slot: u16 offset into `pack_bits`, u16 offset
  into `pack_names`, width, height, solver passes, collection.
- `pack_bits.bin`: rows, MSB first, each row padded to whole bytes (a 20x20
  is 60 bytes, a 15x15 30).
- `pack_names.bin`: upper-case names, NUL-terminated.
- `pack_cinfo.bin`, 8 bytes a collection: u16 first index into
  `pack_cslots`, count, medal number, u16 offset into `pack_cnames`, u16
  emblem slot (the medal picture: the collection's first 15x15 puzzle).
- `pack_minfo.bin`, 8 bytes a mural: u16 first tile slot, tiles across,
  tiles down, u16 offset into `pack_cnames`, two zero bytes. A tile's
  collection byte in `pack_info` is 255.
- `pack_cslots.bin`: u16 slots in each collection's play order.
- `pack_cnames.bin`: collection names.

About 34 KB in all. `puzzle_load` decodes a slot into `g_solution` and
derives the clues at runtime. The same bytes go into `pack_data.c` for the
host tests.

## Checks

`make check` (`gen_assets.py --check`) fails when a puzzle isn't square or
isn't 5/10/15/20, its clues don't fit its layout (docs/LAYOUT.md), its
name is longer than 20 or uses a glyph the font lacks, or the pack outgrows
the save.
