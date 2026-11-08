# ADR-008: Murals
Status: Accepted (2026-11-07)

## Decision
The web game's 16 murals come across: each picture splits into 15x15
tiles (4, 6 or 9 of them, 90 in all), and each tile is a pack puzzle with
its own slot and record. FREE PLAY lists MURALS after the collections; the
murals screen shows the list, and the selected mural's picture as far as
it's solved (one pixel a cell, unsolved tiles outlined). A picks a tile on
the picture, A again plays it. After a solve, A goes on to the mural's next
unsolved tile.

## Why
- Murals are free-play puzzles in the web game (#24 there), not story, so
  "just the puzzles" includes them.
- As pack slots, tiles reuse everything: the play screen, records, the
  suspended board, the result screen. Their collection byte is
  `MURAL_COLLECTION` (255), so collection stats and badges skip them.
- A tile is worth 3 rank points, as in the web game (`TILE_POINTS`); the
  pack plus murals is worth 2,048.

## Details
- `import_pack.py` appends the tiles after the 523 collection puzzles
  (slots 523-612), row by row, named after their mural. Slots stay
  append-only.
- The save grew from 600 slots to 700 (`SAVE_BYTES` 2,976). Nothing had
  shipped, so version 1 changed in place; from the first release on this
  is a layout change (ADR-003).
- `pack_minfo.bin`: per mural, its first tile slot, tiles across and down,
  and its name.
- The preview uses the thumbnail's buffer, grown to 6x6 BG3 tiles (48x48
  pixels, 576 bytes uploaded when it changes). `thumb_tile_bits` draws a
  solved tile straight from its ROM rows.
- Two achievements: MURALIST (finish a mural), THE GALLERY (finish all 16).
  Records shows MURAL TILES solved. PUZZLES SOLVED and the solve-count
  achievements count only the 523 collection puzzles.

## Rejected
- A whole mural as one big scrolling puzzle: the web game plays them tile
  by tile, and a 45x45 board doesn't fit the screen.
- A two-pixel-a-cell preview: 12x12 tiles, too big for the screen beside
  the list.

## Verification
`test/test_core.c` `test_murals`, `test/test_save.c` `test_mural_stats`,
`murals.timeline` (with `murals.pokes`: the preview, a tile, back).
