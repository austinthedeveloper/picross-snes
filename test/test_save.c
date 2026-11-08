/* The battery save, progress records, and the stats derived from them. */
#include <stddef.h>
#include <string.h>
#include "test_support.h"
#include "board.h"
#include "play.h"
#include "progress.h"
#include "puzzle.h"
#include "stats.h"

extern u8 g_host_sram[SAVE_SRAM_BYTES];
extern u16 g_host_tear_after;

static void solve_slot(u16 slot, u8 mode, u16 seconds, u8 hints)
{
    u16 i;

    play_start(slot, mode);
    while (hints--) {
        g_play.cur_x = 0;
        g_play.cur_y = hints;
        play_hint(HINT_ROW);
    }
    for (i = 0; i < g_puzzle.cells; i++)
        if (g_solution[i])
            board_set(i, CELL_FILLED);
        else
            board_set(i, CELL_EMPTY);
    g_play.solved = board_solved();
    g_play.seconds = seconds;
    progress_record_solve();
}

static void test_layout(void)
{
    /* The image must be the same on the SNES (no padding) and the host. */
    CHECK_EQ(sizeof(Record), 4);
    CHECK_EQ(sizeof(Suspend), 112);
    CHECK_EQ(sizeof(SaveData), SAVE_BYTES);
    CHECK_EQ(offsetof(SaveData, sum), 4);
    CHECK_EQ(offsetof(SaveData, suspend), 64);
    CHECK(SAVE_BYTES <= SAVE_COPY_B);
    CHECK(SAVE_COPY_B + SAVE_BYTES <= SAVE_SRAM_BYTES);
}

static void test_blank_and_round_trip(void)
{
    memset(g_host_sram, 0xA5, sizeof g_host_sram);
    CHECK_EQ(save_load(), 0);
    CHECK_EQ(g_save.magic, SAVE_MAGIC);
    CHECK_EQ(g_save.solves, 0);
    solve_slot(0, MODE_RELAXED, 42, 0);
    save_write();
    memset(&g_save, 0, sizeof g_save);
    CHECK_EQ(save_load(), 1);
    CHECK(progress_solved(0));
    CHECK_EQ(progress_record(0)->best, 42);
    CHECK_EQ(g_save.solves, 1);
}

static void test_two_copies(void)
{
    u8 seq;

    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    solve_slot(1, MODE_RELAXED, 10, 0);
    save_write();
    solve_slot(2, MODE_RELAXED, 10, 0);
    save_write();
    seq = g_save.seq;
    /* A torn third write leaves the second intact. */
    solve_slot(3, MODE_RELAXED, 10, 0);
    g_host_tear_after = 100;
    save_write();
    CHECK_EQ(save_load(), 1);
    CHECK_EQ(g_save.seq, seq);
    CHECK(progress_solved(2));
    CHECK(!progress_solved(3));
    /* A flipped bit is caught by the checksum. */
    g_host_sram[SAVE_COPY_B + 200] ^= 1;
    g_host_sram[200] ^= 1;
    CHECK_EQ(save_load(), 0);
}

static void test_sequence_wrap(void)
{
    u16 i;

    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    for (i = 0; i < 300; i++) {
        g_save.solves = i;
        save_write();
        CHECK_EQ(save_load(), 1);
        CHECK_EQ(g_save.solves, i);
    }
}

static void test_records(void)
{
    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    solve_slot(0, MODE_RELAXED, 100, 0);
    CHECK_EQ(progress_record(0)->flags, REC_SOLVED | REC_NOHINT);
    play_start(0, MODE_HEARTS);
    solve_slot(0, MODE_HEARTS, 90, 0);
    CHECK_EQ(progress_record(0)->best, 90);
    CHECK(progress_record(0)->flags & REC_PERFECT);
    solve_slot(0, MODE_HEARTS, 200, 1);
    CHECK_EQ(progress_record(0)->best, 90);
    CHECK_EQ(progress_record(0)->solves, 3);
    CHECK_EQ(g_save.solves, 3);
    CHECK_EQ(g_save.seconds_lo, 390);
}

static void test_suspend(void)
{
    u16 i;
    u8 saved[MAX_CELLS];

    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    play_start(20, MODE_HEARTS);
    for (i = 0; i < g_puzzle.cells; i += 7)
        board_set(i, (u8)(i % 4));
    g_play.seconds = 321;
    g_play.hearts = 2;
    g_play.cur_x = 7;
    g_play.cur_y = 9;
    memcpy(saved, g_cells, sizeof saved);
    progress_suspend();
    save_write();
    save_load();
    play_start(0, MODE_RELAXED);
    CHECK_EQ(progress_resume(19), 0);
    CHECK_EQ(progress_resume(20), 1);
    CHECK_EQ(g_puzzle.slot, 20);
    CHECK_EQ(g_play.mode, MODE_HEARTS);
    CHECK_EQ(g_play.seconds, 321);
    CHECK_EQ(g_play.hearts, 2);
    CHECK_EQ(g_play.cur_x, 7);
    CHECK_EQ(g_play.cur_y, 9);
    CHECK(memcmp(saved, g_cells, g_puzzle.cells) == 0);
    /* Solving it clears the suspended board. */
    solve_slot(20, MODE_HEARTS, 400, 0);
    CHECK_EQ(g_save.suspend.slot1, 0);
}

static void test_stats(void)
{
    u8 i;
    Stats s;

    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    stats_compute(&s);
    CHECK_EQ(s.solved, 0);
    CHECK_EQ(s.rank, 0);
    CHECK_EQ(achievements_done(&s), 0);
    /* Solve the whole starter set in relaxed mode: a bronze badge. */
    for (i = 0; i < collection_size(0); i++)
        solve_slot(collection_slot(0, i), MODE_RELAXED, 600, 0);
    stats_compute(&s);
    CHECK_EQ(s.solved, collection_size(0));
    CHECK_EQ(s.tiers[0], TIER_BRONZE);
    CHECK_EQ(s.badges, 1);
    CHECK_EQ(s.collection_solved[0], collection_size(0));
    CHECK(achievement_done(0, &s));
    CHECK(achievement_done(1, &s));
    CHECK(!achievement_done(2, &s));
    CHECK(achievement_done(14, &s));
    CHECK(achievement_done(11, &s));
    CHECK(!achievement_done(12, &s));
    CHECK(s.rank >= 2);
    /* Perfect hearts solves of all of them: gold. */
    for (i = 0; i < collection_size(0); i++)
        solve_slot(collection_slot(0, i), MODE_HEARTS, 200, 0);
    stats_compute(&s);
    CHECK_EQ(s.tiers[0], TIER_GOLD);
    CHECK_EQ(s.golds, 1);
    CHECK(achievement_done(16, &s));
    CHECK(achievement_done(12, &s));
    CHECK(achievement_done(8, &s));
}

static void test_mural_stats(void)
{
    Stats s;
    u8 t;
    u16 first = mural_first_slot(0);
    u8 n = (u8)(mural_across(0) * mural_down(0));

    memset(g_host_sram, 0, sizeof g_host_sram);
    save_load();
    solve_slot(first, MODE_RELAXED, 100, 0);
    stats_compute(&s);
    /* A tile is worth 3 points but isn't a pack puzzle. */
    CHECK_EQ(s.solved, 0);
    CHECK_EQ(s.tiles_solved, 1);
    CHECK_EQ(s.points, 3);
    CHECK_EQ(s.mural_solved[0], 1);
    CHECK_EQ(s.murals_done, 0);
    CHECK(!achievement_done(0, &s));
    for (t = 1; t < n; t++)
        solve_slot((u16)(first + t), MODE_RELAXED, 100, 0);
    stats_compute(&s);
    CHECK_EQ(s.murals_done, 1);
    CHECK(achievement_done(19, &s));
    CHECK(!achievement_done(20, &s));
    CHECK_EQ(s.badges, 0);
}

static void test_ranks_and_hours(void)
{
    CHECK_EQ(rank_for(0), 0);
    CHECK_EQ(rank_for(9), 0);
    CHECK_EQ(rank_for(10), 1);
    CHECK_EQ(rank_for(1499), 8);
    CHECK_EQ(rank_for(PACK_TOTAL_POINTS), 9);
    g_save.seconds_hi = 0;
    g_save.seconds_lo = 7199;
    CHECK_EQ(progress_hours(), 1);
    /* 3 * 65536 + 1000 s = 197608 s = 54 h 53 min. */
    g_save.seconds_hi = 3;
    g_save.seconds_lo = 1000;
    CHECK_EQ(progress_hours(), 54);
}

int main(void)
{
    test_layout();
    test_blank_and_round_trip();
    test_two_copies();
    test_sequence_wrap();
    test_records();
    test_suspend();
    test_stats();
    test_mural_stats();
    test_ranks_and_hours();
    TEST_END("save");
}
