#include "input.h"
#include "play.h"
#include "progress.h"
#include "puzzle.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

#define LIST_Y 4
#define LIST_ROWS 16
#define DETAIL_Y 20

static u8 s_top;
static u8 s_count;

static const char *const s_bands[5] = {
    "EASY", "MEDIUM", "HARD", "EXPERT", "INSANE"
};

static void draw_row(u8 i)
{
    u8 y;
    u8 on = i == g_sel_index;
    u16 slot;
    u8 band;
    u8 k;
    u8 solved;

    if (i < s_top || i >= s_top + LIST_ROWS)
        return;
    y = (u8)(LIST_Y + i - s_top);
    if (i >= s_count) {
        bg1_fill(1, y, 30, 1, 0);
        bg3_fill(1, y, 30, 1, 0);
        return;
    }
    slot = collection_slot(g_sel_collection, i);
    solved = progress_solved(slot);
    bg1_fill(1, y, 30, 1, on ? SEL_BAR : 0);
    menu_pointer(1, y, on);
    bg3_number(2, y, (u16)(i + 1), 2, 1, TEXT_DIM);
    if (solved)
        bg3_text_n(5, y, (const char *)pack_name(slot), 16,
            on ? TEXT_SELECTED : TEXT_INK);
    else
        bg3_text_n(5, y, "????????", 16, on ? TEXT_SELECTED : TEXT_DIM);
    bg3_number(21, y, pack_width(slot), 2, 0, TEXT_DIM);
    band = pack_band(slot);
    for (k = 0; k < 5; k++)
        g_bg3_map[((u16)y << 5) + 24 + k] = k <= band ?
            (u16)(('~' - 32) | (TEXT_GOLD << 10) | BG_PRIO) :
            (u16)(('`' - 32) | (TEXT_DIM << 10) | BG_PRIO);
    bg3_text(30, y, solved ? "|" : " ", TEXT_OK);
}

static void draw_detail(void)
{
    u16 slot = collection_slot(g_sel_collection, g_sel_index);
    const Record *rec = progress_record(slot);
    u8 x;

    bg3_fill(1, DETAIL_Y + 1, 30, 3, 0);
    if (rec->flags & REC_SOLVED)
        bg3_text(2, DETAIL_Y + 1, (const char *)pack_name(slot), TEXT_INK);
    else
        bg3_text(2, DETAIL_Y + 1, "NOT SOLVED YET", TEXT_DIM);
    if (g_save.suspend.slot1 == slot + 1)
        bg3_text(19, DETAIL_Y + 1, "IN PROGRESS", TEXT_SELECTED);
    bg3_number(2, DETAIL_Y + 2, pack_width(slot), 2, 0, TEXT_DIM);
    bg3_text(4, DETAIL_Y + 2, "X", TEXT_DIM);
    bg3_number(5, DETAIL_Y + 2, pack_height(slot), 2, 0, TEXT_DIM);
    bg3_text(9, DETAIL_Y + 2, s_bands[pack_band(slot)], TEXT_DIM);
    if (rec->flags & REC_SOLVED) {
        bg3_text(17, DETAIL_Y + 2, "BEST", TEXT_DIM);
        bg3_time(22, DETAIL_Y + 2, rec->best, TEXT_INK);
        x = 2;
        bg3_text(x, DETAIL_Y + 3, "SOLVED", TEXT_DIM);
        bg3_number(9, DETAIL_Y + 3, rec->solves, 3, 0, TEXT_INK);
        x = 14;
        if (rec->flags & REC_PERFECT)
            x = bg3_text(x, DETAIL_Y + 3, "~PERFECT ", TEXT_SELECTED);
        else if (rec->flags & REC_HEARTS)
            x = bg3_text(x, DETAIL_Y + 3, "{HEARTS ", TEXT_INK);
        if (rec->flags & REC_NOHINT)
            bg3_text(x, DETAIL_Y + 3, "NO HINTS", TEXT_OK);
    }
}

static void draw_list(void)
{
    u8 i;

    for (i = s_top; i < s_top + LIST_ROWS; i++)
        draw_row(i);
}

static void scroll_to_selection(void)
{
    u8 old = s_top;

    if (g_sel_index < s_top)
        s_top = g_sel_index;
    else if (g_sel_index >= s_top + LIST_ROWS)
        s_top = (u8)(g_sel_index - LIST_ROWS + 1);
    if (old != s_top)
        draw_list();
}

void puzzles_enter(void)
{
    stats_compute(&g_stats);
    bg1_clear();
    bg3_clear();
    s_count = collection_size(g_sel_collection);
    if (g_sel_index >= s_count)
        g_sel_index = 0;
    s_top = 0;
    menu_header((const char *)collection_name(g_sel_collection));
    bg3_number(24, 1, g_stats.collection_solved[g_sel_collection], 2, 0,
        TEXT_INK);
    bg3_text(26, 1, "/", TEXT_DIM);
    bg3_number(27, 1, s_count, 2, 0, TEXT_DIM);
    bg1_frame(0, DETAIL_Y, 32, 5);
    scroll_to_selection();
    draw_list();
    draw_detail();
    bg3_text(1, 26, "A PLAY  B BACK", TEXT_DIM);
    menu_draw_mode(26);
}

void puzzles_update(void)
{
    u8 old = g_sel_index;
    u16 slot;

    if (menu_move(&g_sel_index, s_count) == 0) {
        if (g_repeat & KEY_RIGHT)
            g_sel_index = (u8)(g_sel_index + 10 < s_count ? g_sel_index + 10 : s_count - 1);
        else if (g_repeat & KEY_LEFT)
            g_sel_index = (u8)(g_sel_index >= 10 ? g_sel_index - 10 : 0);
    }
    if (old != g_sel_index) {
        draw_row(old);
        draw_row(g_sel_index);
        scroll_to_selection();
        draw_detail();
    }
    if (menu_mode_toggle())
        menu_draw_mode(26);
    if (g_pressed & KEY_A) {
        slot = collection_slot(g_sel_collection, g_sel_index);
        g_play_slot = slot;
        g_play_resume = g_save.suspend.slot1 == slot + 1;
        scene_goto(SCENE_PLAY);
    } else if (g_pressed & KEY_B) {
        menu_save_settings();
        scene_goto(SCENE_COLLECTIONS);
    }
}
