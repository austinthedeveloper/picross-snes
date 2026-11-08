#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

/* Undo/redo (web: src/engine/history.ts): a ring of cell changes grouped
 * into moves (a stroke, a hint, a clear). When the ring is full the oldest
 * whole move is forgotten. */
#define HISTORY_SIZE 512
#define HISTORY_MASK (HISTORY_SIZE - 1)

void history_reset(void);
/* Starts a new move; the next history_push is its first change. */
void history_begin_move(void);
/* Records a change already applied to the board. Drops any redo. */
void history_push(u16 index, u8 from, u8 to);
/* Undo (redo) one move through board_set. Returns 1 if anything moved. */
u8 history_undo(void);
u8 history_redo(void);
u8 history_can_undo(void);
u8 history_can_redo(void);

#endif
