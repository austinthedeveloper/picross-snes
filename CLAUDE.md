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
- A release: bump `VERSION_NAME`, write `docs/releases/vX.Y.Z.md`, merge,
  then tag `vX.Y.Z` on `main` and push the tag; `release.yml` publishes
  the ROM (ADR-009).
- A feature gets an ADR in `docs/decisions/` (next free number) and a task
  in `milestones/`. A defect gets a row in `docs/KNOWN_ISSUES.md`. Unplanned
  ideas go in `docs/IDEAS.md`.
- The puzzles come from the web game: `make import` re-reads
  `../../picross/src/puzzles` into `data/pack.json` (committed). Never edit
  `data/pack.json` by hand; never reorder its slots (docs/PUZZLES.md).

## Before calling something done

1. `make check`: host tests, pack validation, the 816-tcc lint. Warnings
   are errors on the host.
2. `make debug` builds with no errors, and the free ROM space it prints is
   still positive (grow `ROMBANKS`/`ROMSIZE` together when it isn't).
3. Something visible: run a smoke timeline and look at the screenshots
   (`python3 tools/contact_sheet.py out.png build/smoke/<name>/*.png`).
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

- `make smoke SMOKE=<name> SMOKE_FRAMES=<n>` plays `test/smoke/<name>.timeline`
  on the debug ROM and saves screenshots in `build/smoke/<name>/`. Lines are
  `<first> <last> <buttons>` and `shot <frame>`. Runs start from a blank
  battery save unless `KEEP_SAVE=1`.
- Probing RAM: `SMOKE_PROBES="name=<hex>:<size>"` prints values on shot
  frames; take the address from `build/debug/*.sym` (`awk '$2=="g_x"'`).
  `g_solve_frames` holds the last solve's bookkeeping cost.
- Pillow is not installed; `tools/contact_sheet.py` tiles screenshots and
  `sips -z 448 512 in.png --out out.png` upscales one.
- Sound: `tools/mesen_voices.lua` (same environment variables, no
  `--noAudio`) prints each SPC700 voice start, to show effects really play.
- A `test/smoke/<name>.pokes` file next to a timeline writes RAM by symbol
  before the run plays (`tools/pokes.py`): set up a save instead of playing
  up to it (`badge.pokes` marks 21 starter puzzles solved).
- MesenCE keeps saves in `~/Library/Application Support/MesenCE/Saves/`,
  named after the ROM file.
