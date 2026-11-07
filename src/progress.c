#include "board.h"
#include "history.h"
#include "play.h"
#include "progress.h"

Record *progress_record(u16 slot)
{
    return &g_save.records[slot];
}

u8 progress_solved(u16 slot)
{
    return (g_save.records[slot].flags & REC_SOLVED) != 0;
}

static void add_seconds(u16 seconds)
{
    u16 lo = g_save.seconds_lo + seconds;

    if (lo < g_save.seconds_lo)
        g_save.seconds_hi++;
    g_save.seconds_lo = lo;
}

u8 progress_record_solve(void)
{
    Record *rec = &g_save.records[g_puzzle.slot];
    u8 result = 0;
    u16 seconds = g_play.seconds;

    if ((rec->flags & REC_SOLVED) == 0) {
        result = SOLVE_FIRST;
        rec->best = seconds;
    } else if (seconds < rec->best) {
        result = SOLVE_NEW_BEST;
        rec->best = seconds;
    }
    rec->flags |= REC_SOLVED;
    if (g_play.mode == MODE_HEARTS) {
        rec->flags |= REC_HEARTS;
        g_save.hearts_solves++;
    }
    if (play_perfect()) {
        rec->flags |= REC_PERFECT;
        g_save.perfect_solves++;
    }
    if (g_play.hints_used == 0) {
        rec->flags |= REC_NOHINT;
        g_save.nohint_solves++;
    }
    if (rec->solves < 255)
        rec->solves++;
    g_save.solves++;
    add_seconds(seconds);
    if (g_save.suspend.slot1 == g_puzzle.slot + 1)
        g_save.suspend.slot1 = 0;
    return result;
}

void progress_suspend(void)
{
    Suspend *s = &g_save.suspend;
    u16 i;
    u8 shift = 0;
    u8 byte = 0;
    u8 *out = s->cells;

    s->slot1 = g_puzzle.slot + 1;
    s->seconds = g_play.seconds;
    s->mode = g_play.mode;
    s->hearts = g_play.hearts;
    s->hints_used = g_play.hints_used;
    s->mistakes = g_play.mistakes;
    s->cur_x = g_play.cur_x;
    s->cur_y = g_play.cur_y;
    for (i = 0; i < 100; i++)
        s->cells[i] = 0;
    for (i = 0; i < g_puzzle.cells; i++) {
        byte |= (u8)(g_cells[i] << shift);
        shift += 2;
        if (shift == 8) {
            *out++ = byte;
            byte = 0;
            shift = 0;
        }
    }
    if (shift)
        *out = byte;
}

u8 progress_resume(u16 slot)
{
    Suspend *s = &g_save.suspend;
    u16 i;
    u8 shift = 0;
    const u8 *in = s->cells;

    if (s->slot1 != slot + 1)
        return 0;
    play_start(slot, s->mode);
    for (i = 0; i < g_puzzle.cells; i++) {
        board_set(i, (u8)((*in >> shift) & 3));
        shift += 2;
        if (shift == 8) {
            in++;
            shift = 0;
        }
    }
    g_board_dirty_all = 1;
    g_board_dirty_count = 0;
    g_play.seconds = s->seconds;
    g_play.hearts = s->hearts;
    g_play.hints_used = s->hints_used;
    g_play.mistakes = s->mistakes;
    if (s->cur_x < g_puzzle.width)
        g_play.cur_x = s->cur_x;
    if (s->cur_y < g_puzzle.height)
        g_play.cur_y = s->cur_y;
    history_reset();
    return 1;
}

void progress_clear_suspend(void)
{
    g_save.suspend.slot1 = 0;
}

u16 progress_hours(void)
{
    /* seconds_hi:seconds_lo / 3600 without 32-bit division: each unit of
     * seconds_hi is 65536 s, 18 hours and 736 s. */
    u16 hours = 0;
    u16 rest = g_save.seconds_lo;
    u16 hi = g_save.seconds_hi;

    while (rest >= 3600) {
        rest -= 3600;
        hours++;
    }
    while (hi) {
        hours += 18;
        rest += 736;
        if (rest >= 3600) {
            rest -= 3600;
            hours++;
        }
        hi--;
    }
    return hours;
}
