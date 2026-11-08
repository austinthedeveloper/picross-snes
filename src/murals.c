#include "input.h"
#include "progress.h"
#include "puzzle.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

/* Murals: the list on the left, the selected mural's picture on the right
 * as far as it's solved (one pixel a cell). A picks a tile on the picture,
 * A again plays it. */

#define LIST_Y 4
#define PREVIEW_X 25
#define PREVIEW_Y 5
#define INFO_Y 23

enum { MODE_LIST, MODE_TILE };

static u8 s_mode;
static u8 s_tile;

static void draw_row(u8 m)
{
    u8 y = (u8)(LIST_Y + m);
    u8 on = m == g_sel_mural;
    u8 n = (u8)(mural_across(m) * mural_down(m));
    u8 done = g_stats.mural_solved[m] == n;

    bg1_fill(1, y, 22, 1, on ? SEL_BAR : 0);
    menu_pointer(1, y, on && s_mode == MODE_LIST);
    bg3_text_n(3, y, (const char *)mural_name(m), 15,
        on ? TEXT_SELECTED : done ? TEXT_OK : TEXT_INK);
    bg3_number(19, y, g_stats.mural_solved[m], 1, 0, TEXT_INK);
    bg3_text(20, y, "/", TEXT_DIM);
    bg3_number(21, y, n, 1, 0, TEXT_DIM);
}

/* The picture: solved tiles drawn, the rest outlined. */
static void draw_preview(void)
{
    u8 m = g_sel_mural;
    u8 across = mural_across(m);
    u8 down = mural_down(m);
    u16 slot = mural_first_slot(m);
    u8 tx;
    u8 ty;
    u8 i;
    u8 x0;
    u8 y0;

    thumb_reset((u8)(across * MURAL_TILE), (u8)(down * MURAL_TILE));
    for (ty = 0; ty < down; ty++) {
        for (tx = 0; tx < across; tx++) {
            x0 = (u8)(tx * MURAL_TILE);
            y0 = (u8)(ty * MURAL_TILE);
            if (progress_solved(slot)) {
                thumb_tile_bits(x0, y0, pack_rows(slot));
            } else {
                for (i = 0; i < MURAL_TILE; i++) {
                    thumb_pixel((u8)(x0 + i), y0, 2);
                    thumb_pixel((u8)(x0 + i), (u8)(y0 + MURAL_TILE - 1), 2);
                    thumb_pixel(x0, (u8)(y0 + i), 2);
                    thumb_pixel((u8)(x0 + MURAL_TILE - 1), (u8)(y0 + i), 2);
                }
            }
            slot++;
        }
    }
    bg3_fill(PREVIEW_X, PREVIEW_Y, THUMB_COLS, THUMB_COLS, 0);
    thumb_place(PREVIEW_X, PREVIEW_Y);
}

static void draw_info(void)
{
    u8 m = g_sel_mural;
    u8 n = (u8)(mural_across(m) * mural_down(m));
    u16 slot;
    const Record *rec;
    u8 x;

    bg3_fill(1, INFO_Y, 30, 2, 0);
    if (s_mode == MODE_LIST) {
        x = bg3_text(2, INFO_Y, (const char *)mural_name(m), TEXT_INK);
        if (g_stats.mural_solved[m] == n)
            bg3_text((u8)(x + 1), INFO_Y, "DONE!", TEXT_OK);
        bg3_text(2, INFO_Y + 1, "A: PICK A TILE", TEXT_DIM);
        cursor_hide();
        return;
    }
    slot = (u16)(mural_first_slot(m) + s_tile);
    rec = progress_record(slot);
    x = bg3_text(2, INFO_Y, "TILE", TEXT_DIM);
    bg3_number((u8)(x + 1), INFO_Y, (u16)(s_tile + 1), 1, 0, TEXT_INK);
    bg3_text((u8)(x + 3), INFO_Y, "OF", TEXT_DIM);
    bg3_number((u8)(x + 6), INFO_Y, n, 1, 0, TEXT_INK);
    if (rec->flags & REC_SOLVED) {
        bg3_text(16, INFO_Y, "BEST", TEXT_DIM);
        bg3_time(21, INFO_Y, rec->best, TEXT_INK);
    } else if (g_save.suspend.slot1 == slot + 1) {
        bg3_text(16, INFO_Y, "IN PROGRESS", TEXT_SELECTED);
    }
    bg3_text(2, INFO_Y + 1, "A: PLAY  B: BACK", TEXT_DIM);
    {
        u8 across = mural_across(m);
        u8 ty = 0;
        u8 tx = s_tile;

        while (tx >= across) {
            tx -= across;
            ty++;
        }
        cursor_show_big((u16)(PREVIEW_X * 8 + tx * MURAL_TILE),
            (u16)(PREVIEW_Y * 8 + ty * MURAL_TILE - 1));
    }
}

void murals_enter(void)
{
    u8 m;

    stats_compute(&g_stats);
    bg1_clear();
    bg3_clear();
    menu_header("MURALS");
    bg3_number(24, 1, g_stats.murals_done, 2, 0, TEXT_INK);
    bg3_text(26, 1, "/", TEXT_DIM);
    bg3_number(27, 1, MURAL_COUNT, 2, 0, TEXT_DIM);
    if (g_sel_mural >= MURAL_COUNT)
        g_sel_mural = 0;
    /* Back from a tile: stay on the picture. */
    s_mode = g_return_scene == SCENE_MURALS && s_tile < 9 ? s_mode : MODE_LIST;
    for (m = 0; m < MURAL_COUNT; m++)
        draw_row(m);
    bg1_frame(PREVIEW_X - 1, PREVIEW_Y - 1, THUMB_COLS + 2, THUMB_COLS + 2);
    bg1_frame(0, INFO_Y - 1, 32, 4);
    draw_preview();
    draw_info();
    bg3_text(1, 26, "A CHOOSE  B BACK", TEXT_DIM);
}

void murals_update(void)
{
    u8 old = g_sel_mural;
    u8 across = mural_across(g_sel_mural);
    u8 n = (u8)(across * mural_down(g_sel_mural));
    u16 slot;

    if (s_mode == MODE_LIST) {
        if (menu_move(&g_sel_mural, MURAL_COUNT)) {
            draw_row(old);
            draw_row(g_sel_mural);
            draw_preview();
            draw_info();
        }
        if (g_pressed & KEY_A) {
            s_mode = MODE_TILE;
            s_tile = 0;
            draw_row(g_sel_mural);
            draw_info();
        } else if (g_pressed & KEY_B) {
            scene_goto(SCENE_COLLECTIONS);
        }
        return;
    }
    if (g_repeat & KEY_RIGHT)
        s_tile = (u8)(s_tile + 1 < n ? s_tile + 1 : 0);
    else if (g_repeat & KEY_LEFT)
        s_tile = (u8)(s_tile ? s_tile - 1 : n - 1);
    else if (g_repeat & KEY_DOWN)
        s_tile = (u8)(s_tile + across < n ? s_tile + across : s_tile);
    else if (g_repeat & KEY_UP)
        s_tile = (u8)(s_tile >= across ? s_tile - across : s_tile);
    if (g_repeat & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
        draw_info();
    if (g_pressed & KEY_A) {
        slot = (u16)(mural_first_slot(g_sel_mural) + s_tile);
        g_play_slot = slot;
        g_play_resume = g_save.suspend.slot1 == slot + 1;
        g_return_scene = SCENE_MURALS;
        scene_goto(SCENE_PLAY);
    } else if (g_pressed & KEY_B) {
        s_mode = MODE_LIST;
        draw_row(g_sel_mural);
        draw_info();
    }
}

void murals_init(void)
{
    s_mode = MODE_LIST;
    s_tile = 0;
}

u8 murals_next_tile(void)
{
    /* After a solve: the next unsolved tile of this mural, from the one
     * just played; 0xFF when every tile is solved. */
    u8 n = (u8)(mural_across(g_sel_mural) * mural_down(g_sel_mural));
    u16 first = mural_first_slot(g_sel_mural);
    u8 i;
    u8 t = s_tile;

    for (i = 0; i < n; i++) {
        t = (u8)(t + 1 < n ? t + 1 : 0);
        if (progress_solved((u16)(first + t)) == 0) {
            s_tile = t;
            return t;
        }
    }
    return 0xFF;
}

void murals_select_tile(u8 tile)
{
    s_tile = tile;
    s_mode = MODE_TILE;
}
