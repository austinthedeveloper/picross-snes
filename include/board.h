#ifndef BOARD_H
#define BOARD_H

#include "puzzle.h"

/* A cell the player has set. MISTAKE is a mark the game placed on a wrong
 * fill in hearts mode: it behaves as MARKED and draws in the accent. */
enum { CELL_EMPTY = 0, CELL_FILLED = 1, CELL_MARKED = 2, CELL_MISTAKE = 3 };

extern u8 g_cells[MAX_CELLS];
/* Filled cells that are (are not) filled in the solution. */
extern u16 g_right_filled;
extern u16 g_wrong_filled;

/* Cells changed since the view last drained them (board_view.c). When more
 * than BOARD_DIRTY_MAX change in a frame, g_board_dirty_all is set and the
 * view redraws every cell. */
#define BOARD_DIRTY_MAX 48
extern u16 g_board_dirty[BOARD_DIRTY_MAX];
extern u8 g_board_dirty_count;
extern u8 g_board_dirty_all;

/* Every board_set that changed a cell bumps this, and the value it set is
 * kept, for the play screen's sounds. */
extern u8 g_board_changes;
extern u8 g_board_last_value;

/* Empties every cell of g_puzzle. */
void board_clear(void);
/* Sets a cell, keeping the counts and the dirty list. */
void board_set(u16 index, u8 value);
/* Counts filled cells again after g_cells was written directly. */
void board_recount(void);
u8 board_solved(void);
/* 1 when row r's (column c's) filled runs equal its clues. */
u8 board_row_done(u8 r);
u8 board_col_done(u8 c);

#endif
