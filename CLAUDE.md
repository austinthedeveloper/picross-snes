# Picross '94

A SNES demake of the Picross web game (sibling repo `../../picross`), in C
(816-tcc via PVSnesLib 4.6.0, `PVSNESLIB_HOME`, default `../pvsneslib`) plus
65816 assembly in `src/asm/`. Game rules, saves and stats are plain C that
also builds on the host for tests. MesenCE runs the ROM headless for smoke
runs. Conventions come from Pickleball '94 (`../pickleball`, especially
`docs/NEXT_GAME_PLAYBOOK.md`).

## Workflow

- New work goes on `feature/cld-<topic>`, branched off `main`, pushed to
  GitHub (`austinthedeveloper/picross-snes`) with a PR. CI (ADR-009) must
  pass. The user merges unless they ask otherwise; nothing goes to `main`
  directly.
- Stacked PRs (one based on another's branch) merge bottom-first, each
  retargeted to `main` first: `python3 tools/pr_stack.py merge TOP` does
  it, `pr_stack.py` shows stacks, `pr_stack.py strays` finds merged work
  that never reached `main` (it happened: #35-#37, fixed by #38).
- A release: `python3 tools/release.py X.Y.Z` (branch, version bump,
  drafted notes); merge it; `release.py X.Y.Z --check` says when to tag;
  pushing the tag makes `release.yml` publish the ROM (ADR-009). Smoke-run
  the published ROM (`tools/smoke.py --all --release vX.Y.Z`): CI's build
  differs from a local one byte for byte.
- A feature gets an ADR in `docs/decisions/` (next free number) and a task
  in `milestones/`. A defect gets a row in `docs/KNOWN_ISSUES.md`. Unplanned
  ideas go in `docs/IDEAS.md`.
- The puzzles, art and story come from the web game: `make import` re-reads
  `../../picross` into `data/pack.json`, `data/art/` and `data/story.json`
  (committed; ADR-010). Never edit them by hand; never reorder their slots
  (docs/PUZZLES.md). The daily sync (ADR-011) opens a PR when the web
  changes, or an issue when the story importer finds something to port:
  port it, add it to `SUPPORTED` in `tools/import_story.py`, sync again.
- Story rules mirror the web's `src/game/battle.ts`, `run.ts` and
  `dialogue.ts` function for function (`src/battle.c`, `fight_rules.c`,
  `story_run.c`, `dialogue.c`); a rules change there is a change here.
- `tools/README.md` lists every script.

## Before calling something done

1. `make check`: host tests, pack and story validation, the 816-tcc lint,
   unused functions. Warnings are errors on the host.
2. `make debug` builds with no errors, and the free ROM space it prints is
   still positive (grow `ROMBANKS`/`ROMSIZE` together when it isn't).
3. Something visible: run a smoke timeline and look at its contact sheet
   (`make smoke SMOKE=<name>` writes `build/smoke/<name>.png`), or
   `make smoke-all` before a PR that touches screens.
4. Anything on a solve, a scene change or a save: probe its cost in frames
   (below). The solve bookkeeping is about 7 frames; keep it there.

## Rules of the code

- **816-tcc is slow at some C.** A multiply or divide is a library call; a
  call per element of a 523-puzzle loop costs frames. Walk tables with a
  pointer (`stats_compute`), index lines by shifts (`CLUE_STRIDE` 16), keep
  per-cell work to table lookups (`s_guide`, `g_row_start`).
- **816-tcc miscompiles `a || !b`**: write `b == 0` or split the test.
  `make check` fails on the pattern (`tools/check_tcc_pitfalls.py`).
- **A call inside an array index is an ICE 2** (`a[f(x)]++`): call into a
  local first.
- **Branches are 8-bit in asm**: a `beq` over an unrolled macro fails to
  link ("Too large distance"); branch over a `jmp`.
- **WRAM is not cleared at power-on.** Initialize every static at boot
  (`main`) or scene entry. Mesen prints "Uninitialized memory read" for
  misses; `$0000-$0042` reads are tcc's scratch registers and harmless.
- **Hot or bulk loops go to asm** with the C kept as the host reference:
  `save_sum.asm` (the checksum, 17 frames in C), `map_commit.asm`,
  `sram_copy.asm`.
- **VRAM writes only in VBlank or forced blank.** Draw into the shadow maps
  (`bg1_put`, `bg3_text`); `map_commit` uploads at most 5 dirty 8-row bands
  a frame. Scene changes fade out, redraw in forced blank, and fade in.
- **Every closing press is swallowed** (`input_swallow`): a scene change, a
  menu choice, a hint. The press that closes something never acts on what
  is under it.
- **Save layout changes** follow ADR-003: freeze the old layout as
  `SaveV<n>`, bump `SAVE_VERSION`, migrate in `save.c`, and add a host test
  that loads the old image. Prefer the reserved bytes (0 = default).
- Generated files (`assets/generated/`, `build/`, `hdr.asm`, `linkfile`,
  `*.obj`) stay out of commits.

## Running the ROM headless

- `make smoke SMOKE=<name>` (or `python3 tools/smoke.py <name> ...`) plays
  `test/smoke/<name>.timeline` on the debug ROM: screenshots in
  `build/smoke/<name>/`, a contact sheet `build/smoke/<name>.png`, and a
  failure when it gets fewer screenshots than asked for. Lines are
  `<first> <last> <buttons>` (buttons comma-separated) and `shot <frame>`.
  The run lasts to the last frame + 40 unless `SMOKE_FRAMES`. Runs start
  from a blank battery save unless `KEEP_SAVE=1`. `--list` shows the
  timelines; `--release`, `--run` and `--rom` play a published ROM.
- Probing RAM: `SMOKE_PROBES="name=<hex>:<size>"` prints values on shot
  frames; take the address from `build/debug/*.sym` (`awk '$2=="g_x"'`).
  `g_solve_frames` holds the last solve's bookkeeping cost.
- Pillow is not installed; `tools/png.py` reads and writes PNGs and
  `sips -z 448 512 in.png --out out.png` upscales one.
- Sound: `tools/mesen_voices.lua` (same environment variables, no
  `--noAudio`) prints each SPC700 voice start, to show effects really play.
- A `test/smoke/<name>.pokes` file next to a timeline writes RAM by symbol
  before the run plays (`tools/pokes.py`): set up a save instead of playing
  up to it (`badge.pokes` marks 21 starter puzzles solved).
  `tools/board_pokes.py` writes one that leaves a board a cell short of
  solved (`story_win.pokes`); poke after the play screen opens.
- MesenCE keeps saves in `~/Library/Application Support/MesenCE/Saves/`,
  named after the ROM file.
