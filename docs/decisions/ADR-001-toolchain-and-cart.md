# ADR-001: Toolchain and cartridge
Status: Accepted (2026-11-07)

## Decision
PVSnesLib 4.6.0 (816-tcc, WLA-DX), the same pinned toolchain as Pickleball
'94, at `../pvsneslib`. FastROM LoROM, 128 KiB to start, cartridge type $02
(ROM + RAM + battery) with 8 KiB SRAM. Mode 1: BG1 for the board, BG3 for
text, one sprite.

## Why
- The toolchain, its traps (CLAUDE.md) and the headless MesenCE harness are
  already proven on Pickleball '94; reusing them skips a week of setup.
- A picross game is tiles and text: Mode 1's 16-colour BG1 holds the board
  palettes and medals, and BG3's 4-colour text with the priority bit floats
  over it. No sprite-heavy scenes, so one OBJ palette is enough.
- 8 KiB of SRAM holds two copies of the 2.5 KiB save (ADR-003).

## Details
- Amended 2026-11-07: 256 KiB, for the soundbank's bank (ADR-006).
- The ROM is 98 KiB used at version 0.1.0 (29 KiB of it the pack). Grow
  `ROMBANKS` and `ROMSIZE` together.
- `map_commit.asm` uploads dirty 8-row bands of the shadow maps after
  `WaitForVBlank`, five at most a frame.

## Rejected
- HiROM: no data structure here needs 64 KiB banks.
- Mode 0 (four 4-colour layers): the board needs more than four colours a
  tile (cell, line, guide, ink, mark, clue background).

## Verification
`make debug`; `make smoke SMOKE=solve_heart`.
