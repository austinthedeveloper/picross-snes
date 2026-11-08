# ADR-007: 16-pixel cells for small boards, and a live thumbnail
Status: Accepted (2026-11-07)

## Decision
5x5 and 10x10 boards draw at 16 pixels a cell: every cell and clue is 2x2
BG1 tiles, the clue digits are the 5x7 font at double size (10 in narrow
digits), and the cursor is a 16x16 box. 15x15 and 20x20 stay at 8 pixels.
A thumbnail of the board so far, one pixel a cell, sits in the HUD corner
under the hints.

## Why
- At 8 pixels a 5x5 board was 40 pixels across: the starter set, the first
  thing a player sees, looked like a postage stamp (KNOWN_ISSUES #3).
- 16-pixel cells still fit the pack's worst small-board clues (4 row and 3
  column clues on a 10x10).
- The thumbnail is how the picture reads while you work on a big board; the
  web game shows the board at full size, which the SNES can't for 20x20.

## Details
- `gen_assets.py` draws the big cells and clues as 16x16 pictures and splits
  them into shared, deduplicated tiles; `big_tiles.h` maps (state, guide
  variant) and (striped, number) to four tile numbers. BG1 now holds 496 of
  its 1024 tiles.
- `board_view.c` keeps one code path with a scale shift (`s_big`): cell and
  clue positions shift left by one, the highlight toggles two tile rows or
  columns, the right edge is two tiles tall.
- The thumbnail is nine BG3 tiles after the font (96-104) in a WRAM buffer;
  `thumb_set` flips a pixel's two bit planes and `video_commit` uploads the
  144 bytes in VBlank when they changed. The hint menu and messages cover
  it; closing them puts it back (`view_place_thumb`).

## Rejected
- 16-pixel cells for 15x15 (240 pixels plus clues doesn't fit 256).
- 4 corner sprites for the cursor: the 16x16 box fits the OBJ size already
  in use.

## Verification
`solve_heart.timeline` (5x5 to the result screen), `medium.timeline`
(a 10x10, a stroke, a column hint), `tour.timeline` and `big.timeline`
(8-pixel boards unchanged).
