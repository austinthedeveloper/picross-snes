# Product brief: Picross '94

## What it is

A demake of the Picross web game for the Super Nintendo: what the game
would have been as a 1994 cartridge. The puzzles are the web game's own
523 drawings, imported as data, so every puzzle has the same picture,
clues, name and difficulty in both.

## Version 1: free play

In:

- **Free play.** The 19 collections in the web game's order (the starter
  set, then one theme at a time), each puzzle in its curated position.
  Names stay hidden until a puzzle is solved.
- **Rules from the web game** (`src/game/game.ts` there): relaxed never
  checks fills; hearts checks each fill, marks a wrong one, and ends after
  three. Three hints a puzzle (a cell, a row or a column), undo and redo,
  a clock. Lines whose filled runs match their clues dim.
- **Strokes.** Holding A or B paints across cells, as a drag does in the
  browser; the first cell decides fill or erase, mark or unmark.
- **Tracking.** Every puzzle's best time, times solved, and how it was
  solved (hearts, perfect, without hints); totals; play time; one board
  kept to come back to.
- **Ranks.** The web game's ten ranks and thresholds, by points per solve
  (1/2/3/6 by size; there are no murals or dailies here, so the pack is
  worth 1,778 points and the top rank, 1,500, is still reachable).
- **Badges.** One per collection, in three tiers: bronze for solving all of
  it, silver for solving all of it in hearts mode, gold for solving all of
  it perfectly (hearts mode, no mistakes, no hints).
- **Achievements.** 19 milestones (ADR-004).

Out of version 1 (ADR-002):

- Story mode: jobs, chapters, scenes, fights and enemy powers, gadgets,
  the path map, digs, endless.
- The daily puzzle: a cartridge has no clock to know the date.
- Murals (tiled pictures): a candidate for version 2 (docs/IDEAS.md).
- The editor, pinch zoom, haptics, the high-contrast theme.
- Sound: planned for version 2 (milestones).

## Feel

The web game's look: warm paper, graphite ink, one coral accent, a yellow
highlighter on the row and column under the cursor (docs/LAYOUT.md has the
palette). A 5x7 pixel font with a soft drop shadow.

## Hardware

FastROM LoROM, 128 KiB, 8 KiB battery SRAM (two save copies). Mode 1: BG1
(16 colours) for the board, clues, cards and medals; BG3 (4 colours, high
priority) for text; one sprite for the cursor.
