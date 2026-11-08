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
- **Murals** (ADR-008): the web game's 16 big pictures, 90 tiles, each tile
  a 15x15 puzzle; the picture fills in as you solve its tiles.
- **Tracking.** Every puzzle's best time, times solved, and how it was
  solved (hearts, perfect, without hints); totals; play time; one board
  kept to come back to.
- **Ranks.** The web game's ten ranks and thresholds, by points per solve
  (1/2/3/6 by size; there are no murals or dailies here, so the pack is
  worth 1,778 points, 2,048 with the mural tiles at 3 each; the top rank,
  1,500, is still reachable).
- **Badges.** One per collection, in three tiers: bronze for solving all of
  it, silver for solving all of it in hearts mode, gold for solving all of
  it perfectly (hearts mode, no mistakes, no hints).
- **Achievements.** 21 milestones (ADR-004, ADR-008).
- **Sound effects** (ADR-006), synthesized; an option turns them off.

Out of version 1 (ADR-002):

- Story mode: jobs, chapters, scenes, fights and enemy powers, gadgets,
  the path map, digs, endless.
- The daily puzzle: a cartridge has no clock to know the date.
- The editor, pinch zoom, haptics, the high-contrast theme.

## Feel

The web game's look: warm paper, graphite ink, one coral accent, a yellow
highlighter on the row and column under the cursor (docs/LAYOUT.md has the
palette). A 5x7 pixel font with a soft drop shadow.

## Hardware

FastROM LoROM, 256 KiB, 8 KiB battery SRAM (two save copies). Mode 1: BG1
(16 colours) for the board, clues, cards and medals; BG3 (4 colours, high
priority) for text; one sprite for the cursor.
