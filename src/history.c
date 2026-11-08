#include "board.h"
#include "history.h"

/* Bit 15 of an index marks the first change of a move. */
#define MOVE_START 0x8000
#define INDEX_MASK 0x7FFF

static u16 s_index[HISTORY_SIZE];
static u8 s_from[HISTORY_SIZE];
static u8 s_to[HISTORY_SIZE];
/* Ring position of the oldest change, changes recorded, changes applied. */
static u16 s_start;
static u16 s_count;
static u16 s_pos;
static u8 s_new_move;

void history_reset(void)
{
    s_start = 0;
    s_count = 0;
    s_pos = 0;
    s_new_move = 1;
}

void history_begin_move(void)
{
    s_new_move = 1;
}

/* Forgets the oldest move to make room. */
static void drop_oldest(void)
{
    do {
        s_start = (s_start + 1) & HISTORY_MASK;
        s_count--;
        if (s_pos)
            s_pos--;
    } while (s_count && (s_index[s_start] & MOVE_START) == 0);
}

void history_push(u16 index, u8 from, u8 to)
{
    u16 at;

    if (from == to)
        return;
    s_count = s_pos;
    if (s_count == HISTORY_SIZE)
        drop_oldest();
    at = (s_start + s_count) & HISTORY_MASK;
    s_index[at] = s_new_move ? (index | MOVE_START) : index;
    s_from[at] = from;
    s_to[at] = to;
    s_new_move = 0;
    s_count++;
    s_pos = s_count;
}

u8 history_can_undo(void)
{
    return s_pos != 0;
}

u8 history_can_redo(void)
{
    return s_pos != s_count;
}

u8 history_undo(void)
{
    u16 at;

    if (s_pos == 0)
        return 0;
    do {
        s_pos--;
        at = (s_start + s_pos) & HISTORY_MASK;
        board_set(s_index[at] & INDEX_MASK, s_from[at]);
    } while (s_pos && (s_index[at] & MOVE_START) == 0);
    s_new_move = 1;
    return 1;
}

u8 history_redo(void)
{
    u16 at;

    if (s_pos == s_count)
        return 0;
    do {
        at = (s_start + s_pos) & HISTORY_MASK;
        board_set(s_index[at] & INDEX_MASK, s_to[at]);
        s_pos++;
        at = (s_start + s_pos) & HISTORY_MASK;
    } while (s_pos != s_count && (s_index[at] & MOVE_START) == 0);
    s_new_move = 1;
    return 1;
}
