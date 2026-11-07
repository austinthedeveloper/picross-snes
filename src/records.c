#include "input.h"
#include "progress.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

/* Tracking: rank, totals, solves by size, play time. */

static const char *const s_sizes[4] = { " 5X5", "10X10", "15X15", "20X20" };
static const u16 s_size_counts[4] = {
    PACK_COUNT_5, PACK_COUNT_10, PACK_COUNT_15, PACK_COUNT_20
};

static void row(u8 y, const char *label, u16 value)
{
    bg3_text(3, y, label, TEXT_DIM);
    bg3_number(20, y, value, 5, 0, TEXT_INK);
}

void records_enter(void)
{
    u8 i;
    u8 y;
    u8 next;
    u16 from;
    u16 span;
    u16 into;
    u8 filled;
    u16 seconds;
    u16 minutes;

    stats_compute(&g_stats);
    bg1_clear();
    bg3_clear();
    menu_header("RECORDS");

    bg3_text(3, 4, "RANK", TEXT_DIM);
    bg3_text(8, 4, rank_name(g_stats.rank), TEXT_SELECTED);
    bg3_text(3, 5, "POINTS", TEXT_DIM);
    bg3_number(10, 5, g_stats.points, 4, 0, TEXT_INK);
    next = (u8)(g_stats.rank + 1);
    if (next < RANK_COUNT) {
        bg3_text(16, 5, "NEXT", TEXT_DIM);
        bg3_number(21, 5, rank_points(next), 4, 0, TEXT_INK);
        /* Progress to the next rank as a 20-tile bar. */
        from = rank_points(g_stats.rank);
        span = rank_points(next) - from;
        into = g_stats.points - from;
        filled = (u8)((u16)(into * 20) / span);
    } else {
        bg3_text(16, 5, "TOP RANK!", TEXT_SELECTED);
        filled = 20;
    }
    for (i = 0; i < 20; i++)
        bg1_put((u8)(3 + i), 6, BG_ENTRY(i < filled ? T_BAR_FULL : T_BAR_EMPTY, 2));

    y = 8;
    bg3_text(3, y, "PUZZLES SOLVED", TEXT_DIM);
    bg3_number(18, y, g_stats.solved, 3, 0, TEXT_INK);
    bg3_text(21, y, "/", TEXT_DIM);
    bg3_number(22, y, PACK_LIVE_COUNT, 3, 0, TEXT_DIM);
    y++;
    for (i = 0; i < 4; i++) {
        bg3_text(5, y, s_sizes[i], TEXT_DIM);
        bg3_number(18, y, g_stats.solved_by_size[i], 3, 0, TEXT_INK);
        bg3_text(21, y, "/", TEXT_DIM);
        bg3_number(22, y, s_size_counts[i], 3, 0, TEXT_DIM);
        y++;
    }
    y++;
    row(y++, "TIMES SOLVED", g_save.solves);
    row(y++, "IN HEARTS MODE", g_save.hearts_solves);
    row(y++, "PERFECT", g_save.perfect_solves);
    row(y++, "WITHOUT HINTS", g_save.nohint_solves);
    y++;
    bg3_text(3, y, "PLAY TIME", TEXT_DIM);
    /* Minutes past the hour: the low word's seconds mod 3600, adjusted
     * for each 65536 (736 s past 18 hours). */
    seconds = g_save.seconds_lo;
    for (i = 0; i < g_save.seconds_hi && i < 255; i++) {
        seconds = (u16)(seconds % 3600 + 736);
    }
    minutes = (u16)((seconds % 3600) / 60);
    bg3_number(15, y, g_stats.hours, 4, 0, TEXT_INK);
    bg3_text(19, y, "H", TEXT_DIM);
    bg3_number(21, y, minutes, 2, 1, TEXT_INK);
    bg3_text(23, y, "M", TEXT_DIM);
    y += 2;
    bg3_text(3, y, "BADGES", TEXT_DIM);
    bg3_number(10, y, g_stats.badges, 2, 0, TEXT_INK);
    bg3_text(12, y, "/", TEXT_DIM);
    bg3_number(13, y, COLLECTION_COUNT, 2, 0, TEXT_DIM);
    bg3_text(17, y, "ACHIEVED", TEXT_DIM);
    bg3_number(26, y, achievements_done(&g_stats), 2, 0, TEXT_INK);
    bg3_text(28, y, "/", TEXT_DIM);
    bg3_number(29, y, ACHIEVEMENT_COUNT, 2, 0, TEXT_DIM);
    menu_footer("B BACK");
}

void records_update(void)
{
    if (g_pressed & (KEY_B | KEY_A))
        scene_goto(SCENE_TITLE);
}
