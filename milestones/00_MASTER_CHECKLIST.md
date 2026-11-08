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
- [x] `M2-MURAL-01` Murals
  - Accept: the 16 murals' 90 tiles playable, the picture fills in, rank
    points and two achievements, CONTINUE on a tile
  - Verified: ADR-008, `test_murals`, `test_mural_stats`, `murals.timeline`

## M3: First release

- [x] `M3-CI-01` CI: `make check` and the release ROM on every PR and push
  to `main`, the ROM as an artifact
  - Verified: ADR-009, the CI run on its PR
- [x] `M3-REL-01` A tagged release with the ROM attached
  - Accept: `release.yml` publishes `picross-94-vX.Y.Z.sfc` from a tag
  - Verified: v0.2.0 (`docs/releases/v0.2.0.md`), its ROM smoke-run
- [ ] `M3-PLAY-01` Played through by hand (#19)
- [ ] `M3-HW-01` Tried on real hardware from a flash cart (#20)

## M4: Story mode (ADR-010, ADR-011)

The web game is the source of truth; everything comes in through
`make import`. Rules mirror `src/game/battle.ts`, `run.ts`, `dialogue.ts`
and the engine helpers line for line, with host tests.

- [x] `M4-IMPORT-01` Story importer and compatibility gate
  (`tools/import_story.py`, `data/story.json`)
  - Verified: ADR-010, a planted unsupported field, effect and emoji fail it
- [x] `M4-ART-01` Art import: portraits (64x64), enemy sprites (48x48),
  gadget and node icons (16x16), power glyphs (8x8), 15 colours each
  (`tools/import_art.py`, `data/art/`)
  - Verified: `--check` passes on a re-run; `--preview` looked at
- [x] `M4-SYNC-01` Daily sync workflow: PR when it imports, issue when it
  needs porting (`sync-web.yml`, `tools/sync_summary.py`)
- [x] `M4-DATA-01` Story ROM tables (`tools/gen_story.py`, `src/story.c`):
  chapters, runs, encounters, enemies with phases, scenes, gadgets,
  backdrops; strings in 32 KiB banks; story puzzles from ROM index 700
- [x] `M4-ENGINE-01` Battle rules in C (`src/battle.c`,
  `src/fight_rules.c`): chip, finisher, phases, armor and its hit-back,
  fog, scramble, fakes and call-outs, charge and warm-up, ink, swaps, turns,
  gadgets, shields; the web's line-complete rule (`line_satisfied`)
  - Verified: `test/test_battle.c` against the real story (Gremlin phases,
    Beetle armor, Forger call-outs, Eel charge, Mimic swaps, Weathervane
    turns, fake clues, ink, live line, bombs)
- [x] `M4-DIALOGUE-01` Scene runner (flags, choices, conditions) and the
  scene player: backdrop, portraits on a stage of three, typed text, choices
  (`src/dialogue.c`, `src/scene_view.c`, `src/art.c`; lower case and story
  glyphs in the font; the ROM grows to 512 KiB)
  - Verified: `test/test_story.c` (First Job's intro and choice, the
    outro's won flag, the stage, every scene ends), `scene.timeline`
- [x] `M4-SAVE-01` Save version 2: job results, seen scenes and tips, the
  current run with its board and battle; migration from version 1
  (`save.h StorySave`, `src/story_run.c`: the web's run.ts, runSave.ts and
  unlock rules)
  - Verified: `test_migrate_v1` loads a 0.2.0 image; `test_unlocks_and_runs`
- [x] `M4-JOBS-01` Jobs board by chapter (unlocks, chapter openings,
  Overtime and boss fights under cleared jobs) and the path map (nodes, HP,
  gadgets) (`src/jobs.c`, `src/map.c`)
  - Verified: `story.timeline`
- [x] `M4-FIGHT-01` The fight screen (`src/story_play.c` on the play
  screen): the corner (enemy, HP, your HP, meters, messages), clue overlays
  (fog blanks, scrambles sorted, forgeries shown) and marks (weak points,
  suspects, wipes, the live line), ink, phase cards with taunts, hits in
  Hits-on runs, swaps and turns after the stroke, call-outs (Y), gadgets
  (SELECT), the finisher, knock-outs
  - Verified: `story.timeline`, `story_win.timeline` (the Goblin to its
    finisher and back to the map); KNOWN_ISSUES #7-#9
- [x] `M4-DIG-01` Digs (the gadget reward) and rest stops (the countdown
  and its heal)
- [x] `M4-RESULT-01` Results with the outro's sign-off, job stats
  (`src/run_result.c`)

## M5: Story extras

- [ ] `M5-OVERTIME-01` Overtime runs (on the board and playable; not
  played through yet)
- [ ] `M5-BOSS-01` Standalone boss fights (on the board; not played
  through yet)
- [ ] `M5-ENDLESS-01` Endless
- [ ] `M5-BADGE-01` The web game's story badges
- [ ] `M5-TIPS-01` Power tips the first time each power appears

