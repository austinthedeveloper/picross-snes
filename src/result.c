#include "audio.h"
#include "board.h"
#include "input.h"
#include "play.h"
#include "progress.h"
#include "puzzle.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

/* After a solve: the picture, its name, the time, and what it earned. */

static const u8 s_tier_pal[4] = { TEXT_DIM, TEXT_BRONZE, TEXT_SILVER, TEXT_GOLD };

/* The picture, scale tiles a cell (2 for 5x5 and 10x10). */
static void draw_picture(u8 x0, u8 y0, u8 scale)
{
    u8 r;
    u8 c;
    u16 entry;

    for (r = 0; r < g_puzzle.height; r++)
        for (c = 0; c < g_puzzle.width; c++) {
            entry = BG_ENTRY(g_solution[g_row_start[r] + c] ? T_PIC_FULL : T_PIC_EMPTY,
                PAL_NORMAL);
            bg1_fill((u8)(x0 + c * scale), (u8)(y0 + r * scale), scale, scale, entry);
        }
}

static void draw_news(u8 i, u8 y)
{
    u8 x;

    switch (g_news_kind[i]) {
    case NEWS_RANK:
        x = bg3_text(2, y, "NEW RANK: ", TEXT_DIM);
        bg3_text(x, y, rank_name(g_news_id[i]), TEXT_SELECTED);
        break;
    case NEWS_BADGE:
        x = bg3_text(2, y, "[ ", s_tier_pal[g_news_value[i]]);
        x = bg3_text(x, y, (const char *)collection_name(g_news_id[i]), TEXT_INK);
        x = bg3_text((u8)(x + 1), y, tier_name(g_news_value[i]),
            s_tier_pal[g_news_value[i]]);
        break;
    case NEWS_ACHIEVEMENT:
        x = bg3_text(2, y, "| ", TEXT_OK);
        bg3_text(x, y, achievement_name(g_news_id[i]), TEXT_INK);
        break;
    }
}

void result_enter(void)
{
    u8 scale = g_puzzle.width <= 10 ? 2 : 1;
    u8 side = (u8)(g_puzzle.width * scale);
    u8 x0 = (u8)((32 - side) >> 1);
    u8 y0 = side == 20 ? 1 : 2;
    u8 y;
    u8 i;
    const Record *rec = progress_record(g_puzzle.slot);

    bg1_clear();
    bg3_clear();
    if (side < 20)
        bg1_frame((u8)(x0 - 1), (u8)(y0 - 1), (u8)(side + 2), (u8)(side + 2));
    draw_picture(x0, y0, scale);
    y = (u8)(y0 + side + 1);
    bg3_center(y++, (const char *)pack_name(g_puzzle.slot), TEXT_INK);
    bg3_text(4, y, "TIME", TEXT_DIM);
    bg3_time(9, y, g_play.seconds, TEXT_INK);
    bg3_text(17, y, "BEST", TEXT_DIM);
    bg3_time(22, y, rec->best, TEXT_INK);
    y++;
    if (play_perfect())
        bg3_center(y, "~ PERFECT! ~", TEXT_SELECTED);
    else if (g_solve_result & SOLVE_NEW_BEST)
        bg3_center(y, "NEW BEST TIME!", TEXT_SELECTED);
    else if (g_solve_result & SOLVE_FIRST)
        bg3_center(y, "FIRST CLEAR!", TEXT_SELECTED);
    y++;
    for (i = 0; i < g_news_count && y < 26; i++)
        draw_news(i, y++);
    bg3_center(27, "A NEXT  B LIST", TEXT_DIM);
    if (g_news_count)
        audio_play(SFX_NEWS, 12);
}

void result_update(void)
{
    if (g_pressed & KEY_A) {
        if ((u8)(g_sel_index + 1) < collection_size(g_sel_collection) &&
            collection_slot(g_sel_collection, g_sel_index) == g_puzzle.slot) {
            g_sel_index++;
            g_play_slot = collection_slot(g_sel_collection, g_sel_index);
            g_play_resume = g_save.suspend.slot1 == g_play_slot + 1;
            scene_goto(SCENE_PLAY);
        } else {
            scene_goto(SCENE_PUZZLES);
        }
    } else if (g_pressed & KEY_B) {
        scene_goto(SCENE_PUZZLES);
    }
}
