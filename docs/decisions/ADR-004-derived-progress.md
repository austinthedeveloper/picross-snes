# ADR-004: Ranks, badges and achievements are derived
Status: Accepted (2026-11-07)

## Decision
Nothing about rank, badges or achievements is stored. `stats_compute`
works them out from the per-puzzle records in one pass, on each screen that
shows them and before and after a solve (the difference is what the result
screen announces).

- **Rank**: the web game's ten ranks and thresholds (`src/puzzles/ranks.ts`
  there). Points per distinct puzzle solved: 1 (5x5), 2 (10x10), 3 (15x15),
  6 (20x20). The pack is worth 1,778.
- **Badges**: one per collection. Bronze: every puzzle in it solved. Silver:
  every one solved in hearts mode. Gold: every one solved perfectly (hearts
  mode, no mistakes, no hints). The medal shows the collection's first
  15x15 picture.
- **Achievements** (19): first solve, 10/50/100/250 solves, the whole pack,
  a 20x20, all four sizes, a perfect solve, 10 perfect, 25 in hearts mode,
  a 15x15+ without hints, a 15x15+ under five minutes, 10 hours played,
  first badge, 10 badges, a gold badge, Local Legend, the top rank.

## Why
- The web game does the same for rank ("worked out from existing solves and
  never stored, so old saves have the right rank with no migration").
- Changing a badge rule or adding an achievement needs no save change.
- Tiers follow the web game's job badges (#108 there: any win bronze, gold
  for a clean Hits-on run), with "clear a job" read as "clear a collection".

## Details
`stats_compute` walks the info table and the records with pointers: with a
call per puzzle it cost about three frames more on the SNES.

## Rejected
- Per-collection badges in the web game's style (a pixel picture per badge
  drawn by hand): 19 new drawings. The medal reuses each collection's own
  art.

## Verification
`test/test_save.c` `test_stats`, `test_ranks_and_hours`.
