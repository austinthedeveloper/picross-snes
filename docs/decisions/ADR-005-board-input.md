# ADR-005: Board input
Status: Accepted (2026-11-07)

## Decision
D-pad moves the cursor, repeating after 14 frames every 4, and wraps at the
edges. A starts a fill stroke (an erase stroke on a filled cell), B a mark
stroke (an unmark stroke on a mark); holding the button paints every cell
the cursor crosses. L undoes a whole stroke, R redoes. X opens the hint menu
(cell, row, column) in the HUD corner. Start pauses, and hides the board.

## Why
- Strokes are how the web game is played with a mouse or a finger; on a pad
  the held button stands in for the held mouse button.
- The first cell decides the stroke (web: drag strokes), so a stroke never
  undoes its own work, and marks are safe from a fill stroke (only the first
  cell, when it's a mark, turns into a fill).
- In hearts mode a wrong fill turns into a red mark, costs a heart, and ends
  the stroke until the button is let go, as in the web game.
- Pausing hides the board so the clock can't be cheated.

## Rejected
- Y as a fast move (five cells a press): wrapping makes long trips short.
- A dedicated erase button: A on a filled cell erases.
