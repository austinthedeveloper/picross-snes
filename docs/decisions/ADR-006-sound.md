# ADR-006: Sound effects
Status: Accepted (2026-11-07)

## Decision
Twelve effects synthesized in Python (`tools/sfx.py`), written as one
Impulse Tracker module that smconv turns into a SNESMOD soundbank at ROM
bank 6 (23 KB). `audio.c` boots the sound CPU, loads every effect once, and
plays them with `spcEffect`; `audio_update` hands queued commands over for
six frames after each one. A SOUND option turns them off.

| Effect  | When                                           |
| ------- | ---------------------------------------------- |
| MOVE    | the cursor steps (quiet)                       |
| FILL    | a cell filled: a pencil tap                    |
| MARK    | an X: two short scratches                      |
| ERASE   | a cell cleared (also undo/redo to empty)       |
| LINE    | a row or column turns finished: two notes      |
| MISTAKE | hearts mode: a wrong fill, a low buzz          |
| HINT    | a hint revealed cells                          |
| SOLVE   | the picture is complete: an arpeggio           |
| NEWS    | the result screen announces a rank/badge/achievement |
| BLIP, CONFIRM, BACK | menus                              |

## Why
- Pickleball '94's pipeline (ADR-029/031 there) is proven: deterministic
  synthesis with no recorded audio or licences, and the same IT writer.
- One soundbank bank and no music: the web game has no music either, and a
  puzzle game wants quiet.
- The ROM grows to 256 KiB to give the soundbank its own bank.

## Details
- The play screen picks the sound from what the frame did to the board
  (`g_board_changes`, `g_board_last_value`) and from `g_view_lines_done`,
  so the rules code stays silent and host-testable.
- Menu sounds for every screen but the board come from one place in
  `main.c` (`menu_sounds`).

## Verification
`tools/mesen_voices.lua` on `solve_heart.timeline`: voices start on the menu
presses, every fill and move, and the solve.
