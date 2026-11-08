#ifndef PROGRESS_H
#define PROGRESS_H

#include "save.h"

/* What record_solve found new about a solve. */
enum { SOLVE_FIRST = 1, SOLVE_NEW_BEST = 2 };

Record *progress_record(u16 slot);
u8 progress_solved(u16 slot);
/* Records the finished g_play solve of g_puzzle. Returns SOLVE_* bits. */
u8 progress_record_solve(void);
/* The board in g_play/g_cells, kept for later (quitting part-way). */
void progress_suspend(void);
/* Puts a suspended board back into play. 0 when there isn't one for slot. */
u8 progress_resume(u16 slot);
void progress_clear_suspend(void);
/* Total play time in whole hours. */
u16 progress_hours(void);

#endif
