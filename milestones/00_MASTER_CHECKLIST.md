# Milestones

## M0: Foundation

- [x] `M0-REPO-01` Repo, toolchain pin, Makefile, docs, CLAUDE.md
  - Verified: `make debug`, ADR-001
- [x] `M0-DATA-01` Import the web pack into `data/pack.json`
  - Accept: 523 puzzles, 19 collections, slots append-only
  - Verified: `make import`, `make check` (`gen_assets.py --check`)
- [x] `M0-TEST-01` Host tests for the core, headless smoke runs, 816-tcc lint
  - Verified: `make check`, `make smoke SMOKE=solve_heart`

## M1: Free play (version 0.1)

- [x] `M1-CORE-01` Puzzle decode, clues, board, undo/redo, play rules
  - Accept: relaxed and hearts rules, strokes, hints, clock as the web game
  - Verified: `test/test_core.c`
- [x] `M1-SAVE-01` Battery save, two copies, records, one suspended board
  - Verified: ADR-003, `test/test_save.c`
- [x] `M1-PLAY-01` The play screen: board, clues, highlight, HUD, hint menu,
  pause, solve, out of hearts
  - Verified: ADR-005, `solve_heart.timeline`
- [x] `M1-MENU-01` Title, collections, puzzle list, result screen
  - Verified: `solve_heart.timeline`
- [x] `M1-TRACK-01` Records screen: rank, totals, sizes, play time
  - Verified: ADR-004
- [x] `M1-BADGE-01` Badge shelf (a medal per collection, three tiers)
  - Verified: ADR-004, `test_stats`
- [x] `M1-ACH-01` Achievements (19) with progress
  - Verified: ADR-004, `test_stats`
- [x] `M1-OPT-01` Options: mode, erase records; how to play

## M2: Polish (version 0.2)

- [x] `M2-SFX-01` Sound effects
  - Accept: board, line, mistake, hint, solve, news and menu sounds; an option
    to turn them off
  - Verified: ADR-006, `mesen_voices.lua` on `solve_heart.timeline`
- [x] `M2-THUMB-01` Live thumbnail in the HUD
  - Verified: `hearts.timeline`, `tour.timeline` (the painted run shows)
- [x] `M2-BIG-01` 16-pixel cells for small boards
  - Accept: 5x5 and 10x10 boards, their clues, cursor, solved picture and
    result picture at double size
  - Verified: ADR-007, `solve_heart.timeline`, `medium.timeline`
- [ ] `M2-MURAL-01` Murals
