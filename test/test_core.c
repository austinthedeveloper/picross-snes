/* Puzzle decoding, the board, undo history, and play rules. */
#include <string.h>
#include "test_support.h"
#include "board.h"
#include "history.h"
#include "play.h"
#include "puzzle.h"

/* Fills every solution cell with strokes, one cell each. */
static void solve_by_hand(void)
{
    u8 r, c;

    for (r = 0; r < g_puzzle.height; r++)
        for (c = 0; c < g_puzzle.width; c++)
            if (g_solution[g_row_start[r] + c] &&
                g_cells[g_row_start[r] + c] != CELL_FILLED) {
                g_play.cur_x = c;
                g_play.cur_y = r;
                play_stroke_begin(1);
                play_stroke_end();
            }
}

static void test_heart(void)
{
    /* heart: .#.#. ##### ##### .###. ..#.. */
    puzzle_load(0);
    CHECK_EQ(g_puzzle.width, 5);
    CHECK_EQ(g_puzzle.height, 5);
    CHECK_EQ(g_puzzle.filled, 16);
    CHECK_EQ(g_solution[0], 0);
    CHECK_EQ(g_solution[1], 1);
    CHECK_EQ(g_row_clue_count[0], 2);
    CHECK_EQ(g_row_clues[0], 1);
    CHECK_EQ(g_row_clues[1], 1);
    CHECK_EQ(g_row_clues[1 << 4], 5);
    CHECK_EQ(g_col_clue_count[0], 1);
    CHECK_EQ(g_col_clues[0], 2);
    CHECK_EQ(g_col_clues[2 << 4], 4);
    CHECK_EQ(g_max_row_clues, 2);
    CHECK(strcmp((const char *)pack_name(0), "HEART") == 0);
    CHECK_EQ(pack_points(0), 1);
    CHECK_EQ(pack_collection(0), 0);
}

static void test_every_puzzle(void)
{
    u16 slot;
    u8 c, i;
    u16 seen = 0;

    for (slot = 0; slot < PUZZLE_COUNT; slot++) {
        u16 sum_rows = 0, sum_cols = 0;
        u8 k;

        puzzle_load(slot);
        CHECK(g_puzzle.width == 5 || g_puzzle.width == 10 ||
            g_puzzle.width == 15 || g_puzzle.width == 20);
        CHECK(g_max_row_clues <= 9 && g_max_col_clues <= 8);
        for (i = 0; i < g_puzzle.height; i++)
            for (k = 0; k < g_row_clue_count[i]; k++)
                sum_rows += g_row_clues[(i << 4) + k];
        for (i = 0; i < g_puzzle.width; i++)
            for (k = 0; k < g_col_clue_count[i]; k++)
                sum_cols += g_col_clues[(i << 4) + k];
        CHECK_EQ(sum_rows, g_puzzle.filled);
        CHECK_EQ(sum_cols, g_puzzle.filled);
        CHECK(pack_name(slot)[0] != 0);
    }
    for (c = 0; c < COLLECTION_COUNT; c++) {
        seen += collection_size(c);
        for (i = 0; i < collection_size(c); i++)
            CHECK_EQ(pack_collection(collection_slot(c, i)), c);
        CHECK(collection_emblem(c) < PUZZLE_COUNT);
    }
    CHECK_EQ(seen, PUZZLE_COUNT);
    CHECK(strcmp((const char *)collection_name(0), "STARTER SET") == 0);
}

static void test_relaxed_solve(void)
{
    play_start(0, MODE_RELAXED);
    CHECK(!g_play.solved);
    /* A wrong fill in relaxed mode is just a fill. */
    g_play.cur_x = 0;
    g_play.cur_y = 0;
    play_stroke_begin(1);
    play_stroke_end();
    CHECK_EQ(g_cells[0], CELL_FILLED);
    CHECK_EQ(g_wrong_filled, 1);
    CHECK_EQ(g_play.hearts, MAX_HEARTS);
    /* A on a filled cell erases it. */
    play_stroke_begin(1);
    play_stroke_end();
    CHECK_EQ(g_cells[0], CELL_EMPTY);
    solve_by_hand();
    CHECK(g_play.solved);
    CHECK(board_row_done(0));
    CHECK(board_col_done(4));
    CHECK(!play_perfect());
}

static void test_strokes(void)
{
    play_start(0, MODE_RELAXED);
    /* Drag a fill across row 1 (all filled in the heart). */
    g_play.cur_y = 1;
    g_play.cur_x = 0;
    play_stroke_begin(1);
    for (g_play.cur_x = 1; g_play.cur_x < 5; g_play.cur_x++)
        play_stroke_continue();
    play_stroke_end();
    CHECK_EQ(g_right_filled, 5);
    CHECK(board_row_done(1));
    /* One undo takes back the whole stroke; redo puts it back. */
    CHECK(play_undo());
    CHECK_EQ(g_right_filled, 0);
    CHECK(!board_row_done(1));
    CHECK(play_redo());
    CHECK_EQ(g_right_filled, 5);
    /* A mark stroke skips filled cells; B on a mark starts an unmark. */
    g_play.cur_y = 0;
    g_play.cur_x = 0;
    play_stroke_begin(0);
    g_play.cur_y = 1;
    play_stroke_continue();
    play_stroke_end();
    CHECK_EQ(g_cells[0], CELL_MARKED);
    CHECK_EQ(g_cells[5], CELL_FILLED);
    g_play.cur_y = 0;
    play_stroke_begin(0);
    play_stroke_end();
    CHECK_EQ(g_cells[0], CELL_EMPTY);
    /* A on a mark fills it. */
    g_play.cur_x = 1;
    play_stroke_begin(0);
    play_stroke_end();
    CHECK_EQ(g_cells[1], CELL_MARKED);
    play_stroke_begin(1);
    play_stroke_end();
    CHECK_EQ(g_cells[1], CELL_FILLED);
    /* Undoing that fill brings the mark back in one step. */
    play_undo();
    CHECK_EQ(g_cells[1], CELL_MARKED);
    /* A new move after undo drops the redo. */
    g_play.cur_x = 3;
    play_stroke_begin(1);
    play_stroke_end();
    CHECK(!history_can_redo());
}

static void test_hearts(void)
{
    play_start(0, MODE_HEARTS);
    /* (0,0) is empty in the heart: filling it is a mistake. */
    g_play.cur_x = 0;
    g_play.cur_y = 0;
    play_stroke_begin(1);
    CHECK_EQ(g_cells[0], CELL_MISTAKE);
    CHECK_EQ(g_play.hearts, 2);
    CHECK_EQ(g_play.stroke, STROKE_BLOCKED);
    /* The blocked stroke paints nothing more. */
    g_play.cur_x = 1;
    play_stroke_continue();
    CHECK_EQ(g_cells[1], CELL_EMPTY);
    play_stroke_end();
    /* A mistake mark can be cleared with B. */
    g_play.cur_x = 0;
    play_stroke_begin(0);
    play_stroke_end();
    CHECK_EQ(g_cells[0], CELL_EMPTY);
    /* Two more mistakes fail the puzzle. */
    play_stroke_begin(1);
    play_stroke_end();
    g_play.cur_x = 4;
    play_stroke_begin(1);
    play_stroke_end();
    CHECK_EQ(g_play.hearts, 0);
    CHECK(g_play.failed);
    /* Nothing moves after failing. */
    g_play.cur_x = 1;
    play_stroke_begin(1);
    CHECK_EQ(g_cells[1], CELL_EMPTY);
    play_restart();
    CHECK(!g_play.failed);
    CHECK_EQ(g_play.hearts, MAX_HEARTS);
    CHECK_EQ(g_cells[0], CELL_EMPTY);
    solve_by_hand();
    CHECK(g_play.solved);
    CHECK(play_perfect());
}

static void test_hints(void)
{
    play_start(0, MODE_HEARTS);
    g_play.cur_x = 0;
    g_play.cur_y = 0;
    CHECK_EQ(play_hint(HINT_ROW), 5);
    CHECK_EQ(g_cells[0], CELL_MARKED);
    CHECK_EQ(g_cells[1], CELL_FILLED);
    CHECK_EQ(g_play.hints_used, 1);
    /* A hint that changes nothing is free. */
    CHECK_EQ(play_hint(HINT_ROW), 0);
    CHECK_EQ(g_play.hints_used, 1);
    CHECK(play_hint(HINT_COL) > 0);
    g_play.cur_x = 2;
    g_play.cur_y = 2;
    CHECK_EQ(play_hint(HINT_CELL), 1);
    CHECK_EQ(g_play.hints_used, 3);
    g_play.cur_y = 3;
    CHECK_EQ(play_hint(HINT_ROW), 0);
    /* Undo takes a hint back as one move. */
    CHECK(play_undo());
    CHECK_EQ(g_cells[g_row_start[2] + 2], CELL_EMPTY);
    solve_by_hand();
    CHECK(g_play.solved);
    CHECK(!play_perfect());
}

static void test_history_ring(void)
{
    u16 i;

    /* More changes than the ring holds: the oldest moves go, undo still
     * works back to the oldest kept move. */
    play_start(19, MODE_RELAXED);
    CHECK_EQ(g_puzzle.width, 20);
    for (i = 0; i < 700; i++) {
        g_play.cur_x = (u8)(i % 20);
        g_play.cur_y = (u8)((i / 20) % 20);
        play_stroke_begin(0);
        play_stroke_end();
    }
    for (i = 0; i < 600; i++)
        if (!play_undo())
            break;
    CHECK_EQ(i, HISTORY_SIZE);
    CHECK(!history_can_undo());
}

static void test_clock(void)
{
    u16 i;

    play_start(0, MODE_RELAXED);
    for (i = 0; i < 125; i++)
        play_tick();
    CHECK_EQ(g_play.seconds, 2);
    solve_by_hand();
    for (i = 0; i < 120; i++)
        play_tick();
    CHECK_EQ(g_play.seconds, 2);
}

int main(void)
{
    test_heart();
    test_every_puzzle();
    test_relaxed_solve();
    test_strokes();
    test_hearts();
    test_hints();
    test_history_ring();
    test_clock();
    TEST_END("core");
}
