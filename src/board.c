#include "board.h"

u8 g_cells[MAX_CELLS];
u16 g_right_filled;
u16 g_wrong_filled;
u16 g_board_dirty[BOARD_DIRTY_MAX];
u8 g_board_dirty_count;
u8 g_board_dirty_all;
u8 g_board_changes;
u8 g_board_last_value;

void board_clear(void)
{
    u16 i;

    for (i = 0; i < g_puzzle.cells; i++)
        g_cells[i] = CELL_EMPTY;
    g_right_filled = 0;
    g_wrong_filled = 0;
    g_board_dirty_count = 0;
    g_board_dirty_all = 1;
}

void board_set(u16 index, u8 value)
{
    u8 old = g_cells[index];

    if (old == value)
        return;
    if (old == CELL_FILLED) {
        if (g_solution[index])
            g_right_filled--;
        else
            g_wrong_filled--;
    }
    if (value == CELL_FILLED) {
        if (g_solution[index])
            g_right_filled++;
        else
            g_wrong_filled++;
    }
    g_cells[index] = value;
    g_board_changes++;
    g_board_last_value = value;
    if (g_board_dirty_count < BOARD_DIRTY_MAX)
        g_board_dirty[g_board_dirty_count++] = index;
    else
        g_board_dirty_all = 1;
}

u8 board_solved(void)
{
    return g_wrong_filled == 0 && g_right_filled == g_puzzle.filled;
}

/* Compares one line's filled runs with its clues. */
static u8 line_done(const u8 *cells, u16 step, u8 length, const u8 *clues,
    u8 count)
{
    u8 k = 0;
    u8 run = 0;
    u8 i;

    for (i = 0; i < length; i++) {
        if (*cells == CELL_FILLED) {
            run++;
        } else if (run) {
            if (k >= count || clues[k] != run)
                return 0;
            k++;
            run = 0;
        }
        cells += step;
    }
    if (run) {
        if (k >= count || clues[k] != run)
            return 0;
        k++;
    }
    if (k == 0)
        return clues[0] == 0;
    return k == count;
}

u8 board_row_done(u8 r)
{
    return line_done(g_cells + g_row_start[r], 1, g_puzzle.width,
        g_row_clues + ((u16)r << 4), g_row_clue_count[r]);
}

u8 board_col_done(u8 c)
{
    return line_done(g_cells + c, g_puzzle.width, g_puzzle.height,
        g_col_clues + ((u16)c << 4), g_col_clue_count[c]);
}
