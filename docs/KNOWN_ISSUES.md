# Known issues

| # | Severity | Issue | Reproduction | Status / reason |
| - | -------- | ----- | ------------ | --------------- |
| 1 | Minor | Mesen reports reads of compiler scratch registers (`$00-$42`) before they are written. | `make smoke`, watch stderr. | tcc prologues save these registers; no game state is involved (as Pickleball '94 #6). |
| 2 | Minor | A 20x20 board's column clues can reach screen row 0, which some CRTs crop. Only 3 puzzles have 8 column clues. | Any 20x20 with 8 clues in a column (e.g. check `data/pack.json`). | Open. A 4-pixel vertical scroll for 20x20 would help (docs/IDEAS.md). |
| 3 | Minor | 5x5 and 10x10 boards use 8-pixel cells and look small. | Starter set, first puzzles. | Open: 16-pixel cells for small boards are in docs/IDEAS.md. |
| 4 | Minor | Scrolling a puzzle list redraws all 16 rows: about 5 frames, so holding down past the bottom scrolls a row every 6 frames or so. | A collection with more than 16 puzzles, hold down. | Open. Rows went from 0.6 to 0.3 frames each with `map_draw.asm`; the rest is 816-tcc C in `draw_row` (numbers, stars). |
| 5 | Minor | Opening a screen spends 8-19 frames in forced blank (the play screen the most: stats and 400 cells), on top of the fades. | Any scene change; probe `lag_frame_counter`. | Open. Black, not broken; the play screen could draw cells in asm. |
