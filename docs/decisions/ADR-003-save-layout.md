# ADR-003: Battery save layout
Status: Accepted (2026-11-07)

## Decision
`SaveData` (include/save.h), 2,976 bytes, stored twice in SRAM (offsets
$0000 and $1000). Each copy has a magic ($5850), a version, a sequence
byte and a checksum. A write goes to the older copy; a load takes the newest
valid copy (sequence compared with wrap-around). Records are indexed by pack
slot (docs/PUZZLES.md), with room for 700.

Per slot, 4 bytes: best time in seconds, flags (solved, in hearts mode,
perfect, without hints), times solved (to 255). Plus totals (seconds as two
u16s, solves, hearts solves, perfect solves, solves without hints), the
mode option, one suspended board (2 bits a cell), and 36 reserved bytes.

## Why
- Pickleball '94's recipe (its playbook, §6): two copies with sequence and
  checksum mean a power cut during a write loses only that write.
- Slots, not ids: ids are strings and the ROM has no room for a map. Slots
  are append-only, so a save from any earlier pack still lines up.
- Only one suspended board: the web game keeps a board per puzzle, but 523
  boards would need 52 KiB. Quitting a started puzzle keeps it; quitting
  another started puzzle replaces it.
- Totals as u16 pairs: no 32-bit math on the 65816 and the host and SNES
  layouts stay identical (a host test checks `sizeof` and offsets).

## Details
- Writes happen at safe points only: a solve, leaving a puzzle, closing a
  menu whose settings changed, erasing records. A write costs about four
  frames (the checksum in `save_sum.asm`, then an MVN copy).
- Version 1 is the only layout. The next change freezes it as `SaveV1`,
  bumps `SAVE_VERSION`, migrates in `save.c`'s `migrate`, and adds a host
  test loading a version 1 image. New fields should use the reserved bytes
  (0 meaning the default) and avoid a bump.

## Amendments
- 2026-11-07: 700 slots (2,976 bytes) for the mural tiles (ADR-008).
  Unreleased, so version 1 changed in place.

## Rejected
- Storing earned badges and achievements: they are derived (ADR-004).

## Verification
`test/test_save.c`: layout, round trip, torn write, flipped bit, sequence
wrap, records, suspend and resume.
