#include "board.h"
#include "input.h"
#include "pack.h"
#include "play.h"
#include "progress.h"
#include "puzzle.h"
#include "scene.h"
#include "tiles.h"
#include "video.h"

enum {
    ITEM_CONTINUE, ITEM_FREE_PLAY, ITEM_RECORDS, ITEM_BADGES,
    ITEM_ACHIEVEMENTS, ITEM_HOW_TO, ITEM_OPTIONS, ITEM_COUNT
};
static const char *const s_items[ITEM_COUNT] = {
    "CONTINUE", "FREE PLAY", "RECORDS", "BADGES", "ACHIEVEMENTS",
    "HOW TO PLAY", "OPTIONS"
};
static const u8 s_scenes[ITEM_COUNT] = {
    SCENE_PLAY, SCENE_COLLECTIONS, SCENE_RECORDS, SCENE_BADGES,
    SCENE_ACHIEVEMENTS, SCENE_HOW_TO, SCENE_OPTIONS
};
static const char s_logo[] = LOGO_ROWS;

#define LOGO_X ((32 - LOGO_W) / 2)
#define LOGO_Y 4
#define MENU_Y 12
#define MENU_X 11

u8 g_title_sel;
#define s_sel g_title_sel
static u8 s_first;

static void draw_logo(void)
{
    u8 r;
    u8 c;
    u8 on;
    u16 tile;
    const char *p = s_logo;

    for (r = 0; r < LOGO_H; r++) {
        for (c = 0; c < LOGO_W; c++) {
            on = *p++ == '#' ? 1 : 0;
            if (r == LOGO_H - 1)
                tile = T_CELL_B + (on << 1) + (c % 5 == 0 ? 1 : 0);
            else
                tile = T_CELL + (on << 2) + (c % 5 == 0 ? 1 : 0) +
                    (r == 0 ? 2 : 0);
            bg1_put((u8)(LOGO_X + c), (u8)(LOGO_Y + r), BG_ENTRY(tile, 2));
        }
        bg1_put(LOGO_X + LOGO_W, (u8)(LOGO_Y + r),
            BG_ENTRY(r == LOGO_H - 1 ? T_EDGE_RB : T_EDGE_R, 2));
    }
}

static void draw_items(void)
{
    u8 i;
    u8 y = MENU_Y;

    for (i = s_first; i < ITEM_COUNT; i++) {
        menu_pointer(MENU_X - 2, y, i == s_sel);
        bg3_text(MENU_X, y, s_items[i], i == s_sel ? TEXT_SELECTED : TEXT_INK);
        y += 2;
    }
}

void title_enter(void)
{
    bg1_clear();
    bg3_clear();
    draw_logo();
    bg3_center(LOGO_Y + LOGO_H + 1, "'94", TEXT_SELECTED);
    s_first = g_save.suspend.slot1 ? ITEM_CONTINUE : ITEM_FREE_PLAY;
    if (s_sel < s_first || s_sel >= ITEM_COUNT)
        s_sel = s_first;
    draw_items();
    bg3_center(26, BUILD_ID_STRING, TEXT_DIM);
}

void title_update(void)
{
    u8 rel = (u8)(s_sel - s_first);

    if (menu_move(&rel, (u8)(ITEM_COUNT - s_first))) {
        s_sel = (u8)(rel + s_first);
        draw_items();
    }
    if (g_pressed & (KEY_A | KEY_START)) {
        if (s_sel == ITEM_CONTINUE) {
            u8 i;

            g_play_slot = g_save.suspend.slot1 - 1;
            g_play_resume = 1;
            /* Back out to the puzzle's own list afterwards. */
            g_sel_collection = pack_collection(g_play_slot);
            if (g_sel_collection == MURAL_COLLECTION) {
                g_sel_mural = mural_of(g_play_slot, &i);
                murals_select_tile(i);
                g_sel_collection = COLLECTION_COUNT;
                g_return_scene = SCENE_MURALS;
            } else {
                for (i = 0; i < collection_size(g_sel_collection); i++)
                    if (collection_slot(g_sel_collection, i) == g_play_slot)
                        g_sel_index = i;
                g_return_scene = SCENE_PUZZLES;
            }
        }
        scene_goto(s_scenes[s_sel]);
    }
}
