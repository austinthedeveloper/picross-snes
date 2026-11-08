#include "input.h"
#include "puzzle.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

/* The badge shelf: a medal per collection, coloured by its tier. */

#define PER_ROW 5
#define GRID_X 2
#define GRID_Y 4
#define INFO_Y 23

static u8 s_sel;
static const u8 s_medal_pal[4] = { PAL_LOCKED, PAL_BRONZE, PAL_SILVER, PAL_GOLD };
static const u8 s_text_pal[4] = { TEXT_DIM, TEXT_BRONZE, TEXT_SILVER, TEXT_GOLD };

static u8 medal_x(u8 i)
{
    return (u8)(GRID_X + (i % PER_ROW) * 6);
}

static u8 medal_y(u8 i)
{
    return (u8)(GRID_Y + (i / PER_ROW) * 5);
}

static void draw_medal(u8 i)
{
    u8 x = medal_x(i);
    u8 y = medal_y(i);
    u16 tile = T_MEDAL + ((u16)i << 4);
    u8 pal = s_medal_pal[g_stats.tiers[i]];
    u8 tx;
    u8 ty;

    for (ty = 0; ty < 4; ty++)
        for (tx = 0; tx < 4; tx++)
            bg1_put((u8)(x + tx), (u8)(y + ty), BG_ENTRY(tile++, pal));
}

static void draw_pointer(u8 i, u8 on)
{
    bg3_text((u8)(medal_x(i) - 1), (u8)(medal_y(i) + 1), on ? "^" : " ",
        TEXT_SELECTED);
}

static void draw_info(void)
{
    u8 tier = g_stats.tiers[s_sel];
    u8 x;

    bg3_fill(1, INFO_Y, 30, 3, 0);
    x = bg3_text(2, INFO_Y, (const char *)collection_name(s_sel), TEXT_INK);
    bg3_text((u8)(x + 1), INFO_Y, tier_name(tier), s_text_pal[tier]);
    if (tier == TIER_NONE) {
        x = bg3_text(2, INFO_Y + 1, "BRONZE: SOLVE ALL", TEXT_DIM);
        bg3_number((u8)(x + 1), INFO_Y + 1, g_stats.collection_solved[s_sel], 2, 0,
            TEXT_INK);
        bg3_text((u8)(x + 3), INFO_Y + 1, "/", TEXT_DIM);
        bg3_number((u8)(x + 4), INFO_Y + 1, collection_size(s_sel), 2, 0, TEXT_DIM);
    } else if (tier == TIER_BRONZE) {
        bg3_text(2, INFO_Y + 1, "SILVER: SOLVE ALL IN HEARTS", TEXT_DIM);
    } else if (tier == TIER_SILVER) {
        bg3_text(2, INFO_Y + 1, "GOLD: SOLVE ALL PERFECTLY", TEXT_DIM);
    } else {
        bg3_text(2, INFO_Y + 1, "EVERY PUZZLE PERFECT!", TEXT_GOLD);
    }
}

void badges_enter(void)
{
    u8 i;

    stats_compute(&g_stats);
    bg1_clear();
    bg3_clear();
    menu_header("BADGES");
    bg3_number(24, 1, g_stats.badges, 2, 0, TEXT_INK);
    bg3_text(26, 1, "/", TEXT_DIM);
    bg3_number(27, 1, COLLECTION_COUNT, 2, 0, TEXT_DIM);
    if (s_sel >= COLLECTION_COUNT)
        s_sel = 0;
    for (i = 0; i < COLLECTION_COUNT; i++)
        draw_medal(i);
    draw_pointer(s_sel, 1);
    draw_info();
    menu_footer("B BACK");
}

void badges_update(void)
{
    u8 old = s_sel;

    if (g_repeat & KEY_RIGHT)
        s_sel = (u8)(s_sel + 1 < COLLECTION_COUNT ? s_sel + 1 : 0);
    else if (g_repeat & KEY_LEFT)
        s_sel = (u8)(s_sel ? s_sel - 1 : COLLECTION_COUNT - 1);
    else if (g_repeat & KEY_DOWN)
        s_sel = (u8)(s_sel + PER_ROW < COLLECTION_COUNT ? s_sel + PER_ROW : s_sel % PER_ROW);
    else if (g_repeat & KEY_UP)
        s_sel = (u8)(s_sel >= PER_ROW ? s_sel - PER_ROW : s_sel);
    if (old != s_sel) {
        draw_pointer(old, 0);
        draw_pointer(s_sel, 1);
        draw_info();
    }
    if (g_pressed & KEY_B)
        scene_goto(SCENE_TITLE);
}

void badges_init(void)
{
    s_sel = 0;
}
