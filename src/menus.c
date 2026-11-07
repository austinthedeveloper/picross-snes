#include "input.h"
#include "play.h"
#include "save.h"
#include "scene.h"
#include "video.h"

void menu_header(const char *title)
{
    bg1_frame(0, 0, 32, 3);
    bg3_text(2, 1, title, TEXT_INK);
}

void menu_footer(const char *text)
{
    bg3_center(26, text, TEXT_DIM);
}

u8 menu_move(u8 *sel, u8 count)
{
    if (g_repeat & KEY_UP) {
        *sel = *sel ? (u8)(*sel - 1) : (u8)(count - 1);
        return 1;
    }
    if (g_repeat & KEY_DOWN) {
        *sel = (u8)(*sel + 1) < count ? (u8)(*sel + 1) : 0;
        return 1;
    }
    return 0;
}

void menu_pointer(u8 x, u8 y, u8 on)
{
    bg3_text(x, y, on ? "^" : " ", TEXT_SELECTED);
}

void menu_draw_mode(u8 y)
{
    bg3_text(19, y, "SEL:", TEXT_DIM);
    bg3_text(23, y, g_save.mode == MODE_HEARTS ? "{HEARTS" : "RELAXED",
        g_save.mode == MODE_HEARTS ? TEXT_SELECTED : TEXT_INK);
}

static u8 s_settings_dirty;

u8 menu_mode_toggle(void)
{
    if ((g_pressed & KEY_SELECT) == 0)
        return 0;
    g_save.mode = g_save.mode == MODE_HEARTS ? MODE_RELAXED : MODE_HEARTS;
    s_settings_dirty = 1;
    return 1;
}

void menu_settings_changed(void)
{
    s_settings_dirty = 1;
}

void menu_save_settings(void)
{
    if (s_settings_dirty == 0)
        return;
    s_settings_dirty = 0;
    save_write();
}

void menu_init(void)
{
    s_settings_dirty = 0;
}
