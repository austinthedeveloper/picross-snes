# Layout

## Screen and VRAM

Mode 1, 256x224, 32x28 tiles. All in VRAM words:

| What        | Address | Notes                                       |
| ----------- | ------- | ------------------------------------------- |
| BG1 tiles   | $0000   | 4bpp, `board.pic` (368 tiles of 512)        |
| BG3 tiles   | $4000   | 2bpp font, tile = ASCII - 32                |
| BG1 map     | $5000   | 32x32, shadow `g_bg1_map`                   |
| BG3 map     | $5400   | 32x32, shadow `g_bg3_map`, priority bit set |
| OBJ tiles   | $6000   | the cursor (16x16, tiles 0/1/16/17)         |

BG3 has the Mode 1 priority bit, so text draws over the board and the
cursor. BG2 is off.

## Palettes (CGRAM)

- 0: the backdrop, paper.
- BG3 palettes 0-7 (colours 0-31): ink, dim, selected (accent), light,
  ok, bronze, silver, gold. Colour 1 is the glyph, 2 its drop shadow, 3 the
  accent glyphs (heart, star).
- BG1 palettes 2-7 (colours 32-127) share one index layout:
  1 cell, 2 grid line, 3 guide line (every 5th), 4 ink, 5 mark, 6 clue
  background, 8 card, 9 card border, 12-15 medal (dark, light, plate,
  picture).
  - 2 normal, 3 highlighted (1 and 6 turn highlighter yellow), 4 mistake
    (5 turns accent), 5 mistake + highlighted, 6 finished clue (4 turns
    muted), 7 finished + highlighted.
  - Medals reuse 2-5 through colours 12-15: locked, bronze, silver, gold.
- OBJ palette 0: the cursor (accent, ink).

## The board

A cell tile draws its own top and left grid lines (guide colour every
fifth line); the bottom row's tiles also draw the bottom edge, and a column
of edge tiles draws the right edge. So every board fits in its own
width + 1 by height tiles.

The board's top-left tile by size, chosen so the pack's worst clue counts
fit (tools/gen_assets.py `CLUE_ROOM` checks every puzzle):

| Size  | Board at | Row clues room | Column clues room |
| ----- | -------- | -------------- | ----------------- |
| 5x5   | (15, 12) | 5              | 5                 |
| 10x10 | (14, 11) | 8              | 7                 |
| 15x15 | (13, 10) | 8              | 8                 |
| 20x20 | (11, 8)  | 9              | 8                 |

A 20x20 board fills rows 8-27, so its column clues may reach row 0. The
top-left corner left of the column clues and above the row clues is always
free: the HUD (size, time, hearts, hints) and the hint menu live there on
BG3.

Row clues are right-aligned against the board, column clues bottom-aligned
above it, each number in its own tile (1-9 in the 5x7 font, 10-20 in 3x7
narrow digits). A line's whole clue band takes its palette, so the
highlight runs from the board to the far clue.
