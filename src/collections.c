#include "input.h"
#include "puzzle.h"
#include "save.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

#define LIST_Y 4

static const u8 s_tier_pal[4] = { TEXT_DIM, TEXT_BRONZE, TEXT_SILVER, TEXT_GOLD };

static void draw_row(u8 c)
{
    u8 y = (u8)(LIST_Y + c);
    u8 on = c == g_sel_collection;

    bg1_fill(1, y, 30, 1, on ? SEL_BAR : 0);
    menu_pointer(1, y, on);
    bg3_text_n(3, y, (const char *)collection_name(c), 19,
        on ? TEXT_SELECTED : TEXT_INK);
    bg3_number(23, y, g_stats.collection_solved[c], 2, 0, TEXT_INK);
    bg3_text(25, y, "/", TEXT_DIM);
    bg3_number(26, y, collection_size(c), 2, 0, TEXT_DIM);
    if (g_stats.tiers[c])
        bg3_text(29, y, "[", s_tier_pal[g_stats.tiers[c]]);
}

#define MURALS_ROW COLLECTION_COUNT

static void draw_murals_row(void)
{
    u8 y = (u8)(LIST_Y + MURALS_ROW + 1);
    u8 on = g_sel_collection == MURALS_ROW;

    bg1_fill(1, y, 30, 1, on ? SEL_BAR : 0);
    menu_pointer(1, y, on);
    bg3_text_n(3, y, "MURALS", 19, on ? TEXT_SELECTED : TEXT_INK);
    bg3_number(23, y, g_stats.murals_done, 2, 0, TEXT_INK);
    bg3_text(25, y, "/", TEXT_DIM);
    bg3_number(26, y, MURAL_COUNT, 2, 0, TEXT_DIM);
}

static void draw_any(u8 c)
{
    if (c == MURALS_ROW)
        draw_murals_row();
    else
        draw_row(c);
}

void collections_enter(void)
{
    u8 c;

    bg1_clear();
    bg3_clear();
    stats_compute(&g_stats);
    menu_header("FREE PLAY");
    bg3_number(23, 1, g_stats.solved, 3, 0, TEXT_INK);
    bg3_text(26, 1, "/", TEXT_DIM);
    bg3_number(27, 1, PACK_LIVE_COUNT, 3, 0, TEXT_DIM);
    if (g_sel_collection > MURALS_ROW)
        g_sel_collection = 0;
    for (c = 0; c < COLLECTION_COUNT; c++)
        draw_row(c);
    draw_murals_row();
    bg3_text(1, 26, "A OPEN  B BACK", TEXT_DIM);
    menu_draw_mode(26);
}

void collections_update(void)
{
    u8 old = g_sel_collection;

    if (menu_move(&g_sel_collection, COLLECTION_COUNT + 1)) {
        draw_any(old);
        draw_any(g_sel_collection);
    }
    if (menu_mode_toggle())
        menu_draw_mode(26);
    if (g_pressed & KEY_A) {
        g_sel_index = 0;
        if (g_sel_collection == MURALS_ROW) {
            g_return_scene = SCENE_COLLECTIONS;
            scene_goto(SCENE_MURALS);
        } else {
            g_return_scene = SCENE_PUZZLES;
            scene_goto(SCENE_PUZZLES);
        }
    } else if (g_pressed & KEY_B) {
        menu_save_settings();
        scene_goto(SCENE_TITLE);
    }
}
