#include "input.h"
#include "scene.h"
#include "stats.h"
#include "tiles.h"
#include "video.h"

/* Achievements: a list with progress, and the selected one's goal. */

#define LIST_Y 4
#define INFO_Y 24

static u8 s_sel;

static void draw_row(u8 i)
{
    u8 y = (u8)(LIST_Y + i);
    u8 on = i == s_sel;
    u8 done = achievement_done(i, &g_stats);
    u16 value = achievement_value(i, &g_stats);
    u16 target = achievement_target(i);

    if (value > target)
        value = target;
    bg1_fill(1, y, 30, 1, on ? SEL_BAR : 0);
    menu_pointer(1, y, on);
    bg3_text(2, y, done ? "|" : " ", TEXT_OK);
    bg3_text_n(4, y, achievement_name(i), 18,
        on ? TEXT_SELECTED : done ? TEXT_INK : TEXT_DIM);
    bg3_number(22, y, value, 3, 0, done ? TEXT_OK : TEXT_INK);
    bg3_text(25, y, "/", TEXT_DIM);
    bg3_number(26, y, target, 3, 0, TEXT_DIM);
}

static void draw_info(void)
{
    const char *text = achievement_text(s_sel);
    char line[30];
    u8 n;
    u8 y = INFO_Y;

    bg3_fill(1, INFO_Y, 30, 2, 0);
    while (*text && y < INFO_Y + 2) {
        n = 0;
        while (*text && *text != '\n' && n < 29)
            line[n++] = *text++;
        line[n] = 0;
        if (*text == '\n')
            text++;
        bg3_text(2, y++, line, TEXT_INK);
    }
}

void achievements_enter(void)
{
    u8 i;

    stats_compute(&g_stats);
    bg1_clear();
    bg3_clear();
    menu_header("ACHIEVEMENTS");
    bg3_number(24, 1, achievements_done(&g_stats), 2, 0, TEXT_INK);
    bg3_text(26, 1, "/", TEXT_DIM);
    bg3_number(27, 1, ACHIEVEMENT_COUNT, 2, 0, TEXT_DIM);
    if (s_sel >= ACHIEVEMENT_COUNT)
        s_sel = 0;
    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
        draw_row(i);
    bg1_frame(0, INFO_Y - 1, 32, 4);
    draw_info();
}

void achievements_update(void)
{
    u8 old = s_sel;

    if (menu_move(&s_sel, ACHIEVEMENT_COUNT)) {
        draw_row(old);
        draw_row(s_sel);
        draw_info();
    }
    if (g_pressed & KEY_B)
        scene_goto(SCENE_TITLE);
}

void achievements_init(void)
{
    s_sel = 0;
}
