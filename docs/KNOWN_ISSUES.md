# Known issues

| # | Severity | Issue | Reproduction | Status / reason |
| - | -------- | ----- | ------------ | --------------- |
| 1 | Minor | Mesen reports reads of compiler scratch registers (`$00-$42`) before they are written. | `make smoke`, watch stderr. | tcc prologues save these registers; no game state is involved (as Pickleball '94 #6). |
| 2 | Minor | A 20x20 board's column clues can reach screen row 0, which some CRTs crop. Only 3 puzzles have 8 column clues. | Any 20x20 with 8 clues in a column (e.g. check `data/pack.json`). | Open. A 4-pixel vertical scroll for 20x20 would help (docs/IDEAS.md). |
| 3 | Resolved | 5x5 and 10x10 boards used 8-pixel cells and looked small. | Starter set, first puzzles. | Fixed (`feature/cld-board-polish`, ADR-007): 16-pixel cells and clues. |
| 4 | Resolved | Scrolling a puzzle list redrew all 16 rows (about 5 frames), and quick presses during it were lost. | A collection with more than 16 puzzles, tap down quickly past the bottom (`big.timeline` stopped two rows short). | Fixed (`feature/cld-v1-free-play`): a one-row scroll moves the map rows with MVN/MVP (`wram_move.asm`) and draws only the incoming row; `big.timeline` reaches row 20. |
| 6 | Resolved | Two-digit column clues in neighbouring columns ran together ("17 13" read as "1713"). | `big.timeline` (CASTLE). | Fixed (`feature/cld-v1-free-play`): odd clue lines have a darker background (docs/LAYOUT.md). |
| 5 | Minor | Opening a screen spends 8-19 frames in forced blank (the play screen the most: stats and 400 cells), on top of the fades. | Any scene change; probe `lag_frame_counter`. | Open. Black, not broken; the play screen could draw cells in asm. |
| 7 | Minor | Fights show the enemy's name, not its sprite: the corner left of the column clues is too small for a 48x48 sprite beside the HP and meters. The sprites are in ROM (`art.c`). | Any fight. | Open. A sprite (OBJ) over the corner, or a battle bar strip, is in docs/IDEAS.md. |
| 8 | Minor | After a swap, a turn or a bomb, undo starts over; the web remaps its history (`History.remap`). | Undo after the Mimic swaps. | Open. |
| 9 | Minor | A fight with both forgeries and turns makes its forgeries from the turned clues; the web forges the drawn clues and turns them. No fight has both yet. | None in the story. | Open; the importer could reject the pair until it's ported. |

