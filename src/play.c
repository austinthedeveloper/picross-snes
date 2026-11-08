#include "history.h"
#include "play.h"

Play g_play;

#define MAX_SECONDS 59999

static void reset_state(void)
{
    g_play.hearts = MAX_HEARTS;
    g_play.hints_used = 0;
    g_play.mistakes = 0;
    g_play.solved = 0;
    g_play.failed = 0;
    g_play.hits = 0;
    g_play.stroke = STROKE_NONE;
    g_play.frames = 0;
    g_play.seconds = 0;
    board_clear();
    history_reset();
}

void play_start(u16 slot, u8 mode)
{
    puzzle_load(slot);
    g_play.mode = mode;
    g_play.cur_x = 0;
    g_play.cur_y = 0;
    reset_state();
}

void play_restart(void)
{
    reset_state();
}

static u16 cursor_index(void)
{
    return g_row_start[g_play.cur_y] + g_play.cur_x;
}

static u8 over(void)
{
    return g_play.solved || g_play.failed;
}

static void set(u16 index, u8 value)
{
    history_push(index, g_cells[index], value);
    board_set(index, value);
}

/* Applies the current stroke to one cell. */
static void apply(u16 index)
{
    u8 cell = g_cells[index];

    switch (g_play.stroke) {
    case STROKE_FILL:
        if (cell != CELL_EMPTY)
            return;
        if (g_play.mode != MODE_RELAXED && g_solution[index] == 0) {
            set(index, CELL_MISTAKE);
            g_play.mistakes++;
            g_play.stroke = STROKE_BLOCKED;
            if (g_play.mode == MODE_BATTLE) {
                g_play.hits++;
                return;
            }
            g_play.hearts--;
            if (g_play.hearts == 0)
                g_play.failed = 1;
            return;
        }
        set(index, CELL_FILLED);
        break;
    case STROKE_ERASE:
        if (cell == CELL_FILLED)
            set(index, CELL_EMPTY);
        break;
    case STROKE_MARK:
        if (cell == CELL_EMPTY)
            set(index, CELL_MARKED);
        break;
    case STROKE_UNMARK:
        if (cell == CELL_MARKED || cell == CELL_MISTAKE)
            set(index, CELL_EMPTY);
        break;
    }
    if (board_solved())
        g_play.solved = 1;
}

void play_stroke_begin(u8 button_a)
{
    u16 index;
    u8 cell;

    if (over())
        return;
    index = cursor_index();
    cell = g_cells[index];
    history_begin_move();
    if (button_a) {
        if (cell == CELL_FILLED) {
            g_play.stroke = STROKE_ERASE;
        } else {
            g_play.stroke = STROKE_FILL;
            /* Filling over a mark replaces it, for the first cell only. */
            if (cell == CELL_MARKED || cell == CELL_MISTAKE)
                set(index, CELL_EMPTY);
        }
    } else {
        if (cell == CELL_MARKED || cell == CELL_MISTAKE)
            g_play.stroke = STROKE_UNMARK;
        else
            g_play.stroke = STROKE_MARK;
    }
    apply(index);
}

void play_stroke_continue(void)
{
    if (g_play.stroke == STROKE_NONE || g_play.stroke == STROKE_BLOCKED)
        return;
    if (over())
        return;
    apply(cursor_index());
}

void play_stroke_end(void)
{
    g_play.stroke = STROKE_NONE;
    history_begin_move();
}

u8 play_undo(void)
{
    u8 moved;

    if (over())
        return 0;
    moved = history_undo();
    if (board_solved())
        g_play.solved = 1;
    return moved;
}

u8 play_redo(void)
{
    u8 moved;

    if (over())
        return 0;
    moved = history_redo();
    if (board_solved())
        g_play.solved = 1;
    return moved;
}

u8 play_hint(u8 target)
{
    u8 changed = 0;
    u8 i;
    u8 n;
    u16 index;
    u16 step;
    u8 want;

    if (over() || g_play.hints_used >= MAX_HINTS)
        return 0;
    if (target == HINT_ROW) {
        index = g_row_start[g_play.cur_y];
        step = 1;
        n = g_puzzle.width;
    } else if (target == HINT_COL) {
        index = g_play.cur_x;
        step = g_puzzle.width;
        n = g_puzzle.height;
    } else {
        index = cursor_index();
        step = 1;
        n = 1;
    }
    history_begin_move();
    for (i = 0; i < n; i++) {
        want = g_solution[index] ? CELL_FILLED : CELL_MARKED;
        if (g_cells[index] != want) {
            set(index, want);
            changed++;
        }
        index += step;
    }
    history_begin_move();
    if (changed) {
        g_play.hints_used++;
        if (board_solved())
            g_play.solved = 1;
    }
    return changed;
}

void play_tick(void)
{
    if (over())
        return;
    if (++g_play.frames >= 60) {
        g_play.frames = 0;
        if (g_play.seconds < MAX_SECONDS)
            g_play.seconds++;
    }
}

u8 play_perfect(void)
{
    return g_play.mode == MODE_HEARTS && g_play.mistakes == 0 &&
        g_play.hints_used == 0;
}
