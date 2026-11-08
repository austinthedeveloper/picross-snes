# Picross '94

A Super Nintendo demake of the [Picross web game](../../picross), built in C
and 65816 assembly with [PVSnesLib](https://github.com/alekmaul/pvsneslib)
4.6.0. It is the free-play half of that game: all 523 handmade puzzles in
their 19 collections plus the 16 murals, with the tracking, ranks, badges
and achievements around them. Story mode (jobs, fights, scenes, endless,
the daily) is cut from this first version (docs/decisions/ADR-002).

- 523 puzzles from 5×5 to 20×20, each one with a unique solution, drawn in
  the web game and imported as data (`data/pack.json`)
- 16 murals: big pictures in 90 tile puzzles that fill in as you solve them
- Relaxed or Hearts mode, 3 hints a puzzle, undo and redo, a clock
- Hold A or B to paint a stroke across cells, like dragging in the browser
- Records: rank (the same ten as the web game), totals, solves by size,
  play time
- A badge per collection (bronze, silver, gold) and 21 achievements
- Battery save: every record, plus one board to come back to
- Synthesized sound effects (pencil taps, line chimes, a solve jingle)

## Build

Needs PVSnesLib 4.6.0 next to this repo (`../pvsneslib`, or set
`PVSNESLIB_HOME`), Python 3, a host C compiler, and MesenCE for the headless
runs.

```sh
make            # debug ROM in build/debug/
make release    # release ROM in build/release/
make check      # host tests, pack validation, 816-tcc lint
make smoke SMOKE=solve_heart SMOKE_FRAMES=560   # headless run + screenshots
```

Open the `.sfc` in an emulator (Mesen 2 or bsnes). It's a 256 KiB FastROM
LoROM with 8 KiB of battery SRAM.

## Controls

| Button | On the board                   | On the lists          |
| ------ | ------------------------------ | --------------------- |
| D-pad  | Move (repeats, wraps)          | Move                  |
| A      | Fill / erase (hold to paint)   | Choose                |
| B      | Mark X / unmark (hold to paint)| Back                  |
| L / R  | Undo / redo                    | Page (puzzle list: ←/→)|
| X      | Hint: cell, row or column      |                       |
| Start  | Pause: resume, restart, quit   |                       |
| Select |                                | Relaxed / Hearts      |

## Docs

- [CLAUDE.md](CLAUDE.md): workflow, the rules of the code, running headless
- [Product brief](docs/00_PRODUCT_BRIEF.md): what this version is and isn't
- [Layout](docs/LAYOUT.md): screens, VRAM, palettes, the board's placement
- [Puzzles](docs/PUZZLES.md): importing the pack, the ROM format, save slots
- [Decisions](docs/decisions/), [known issues](docs/KNOWN_ISSUES.md),
  [ideas](docs/IDEAS.md), [milestones](milestones/00_MASTER_CHECKLIST.md)
