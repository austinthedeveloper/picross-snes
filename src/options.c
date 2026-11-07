#include "input.h"
#include "play.h"
#include "save.h"
#include "scene.h"
#include "tiles.h"
#include "video.h"

enum { OPT_MODE, OPT_SOUND, OPT_ERASE, OPT_BACK, OPT_COUNT };

static u8 s_sel;
/* Presses of A on ERASE so far (it takes three). */
static u8 s_erase;

static void draw(void)
{
    u8 y = 6;

    menu_pointer(3, y, s_sel == OPT_MODE);
    bg3_text(5, y, "MODE", s_sel == OPT_MODE ? TEXT_SELECTED : TEXT_INK);
    bg3_text(14, y, g_save.mode == MODE_HEARTS ? "< HEARTS > " : "< RELAXED >",
        TEXT_INK);
    bg3_fill(5, 8, 26, 2, 0);
    if (g_save.mode == MODE_HEARTS) {
        bg3_text(5, 8, "WRONG FILLS COST A HEART.", TEXT_DIM);
        bg3_text(5, 9, "THREE AND THE PUZZLE ENDS.", TEXT_DIM);
    } else {
        bg3_text(5, 8, "NO CHECKING. SOLVE AT", TEXT_DIM);
        bg3_text(5, 9, "YOUR OWN PACE.", TEXT_DIM);
    }
    y = 11;
    menu_pointer(3, y, s_sel == OPT_SOUND);
    bg3_text(5, y, "SOUND", s_sel == OPT_SOUND ? TEXT_SELECTED : TEXT_INK);
    bg3_text(14, y, g_save.sound ? "< ON  >" : "< OFF >", TEXT_INK);
    y = 14;
    menu_pointer(3, y, s_sel == OPT_ERASE);
    bg3_text(5, y, "ERASE ALL RECORDS", s_sel == OPT_ERASE ? TEXT_SELECTED : TEXT_INK);
    bg3_fill(5, 15, 26, 1, 0);
    if (s_erase == 1)
        bg3_text(5, 15, "PRESS A AGAIN TO CONFIRM", TEXT_SELECTED);
    else if (s_erase == 2)
        bg3_text(5, 15, "REALLY? A ONCE MORE.", TEXT_SELECTED);
    else if (s_erase == 3)
        bg3_text(5, 15, "RECORDS ERASED.", TEXT_DIM);
    y = 18;
    menu_pointer(3, y, s_sel == OPT_BACK);
    bg3_text(5, y, "BACK", s_sel == OPT_BACK ? TEXT_SELECTED : TEXT_INK);
}

void options_enter(void)
{
    bg1_clear();
    bg3_clear();
    menu_header("OPTIONS");
    s_sel = OPT_MODE;
    s_erase = 0;
    draw();
    menu_footer("A CHOOSE  B BACK");
}

void options_update(void)
{
    if (menu_move(&s_sel, OPT_COUNT)) {
        if (s_erase != 3)
            s_erase = 0;
        draw();
    }
    if (s_sel == OPT_MODE && (g_pressed & (KEY_LEFT | KEY_RIGHT | KEY_A))) {
        g_save.mode = g_save.mode == MODE_HEARTS ? MODE_RELAXED : MODE_HEARTS;
        menu_settings_changed();
        draw();
    } else if (s_sel == OPT_SOUND && (g_pressed & (KEY_LEFT | KEY_RIGHT | KEY_A))) {
        g_save.sound = g_save.sound ? 0 : 1;
        menu_settings_changed();
        draw();
    } else if (s_sel == OPT_ERASE && (g_pressed & KEY_A) && s_erase < 3) {
        s_erase++;
        if (s_erase == 3) {
            u8 mode = g_save.mode;
            u8 sound = g_save.sound;

            save_blank();
            g_save.mode = mode;
            g_save.sound = sound;
            /* Both copies, so the old records can't come back. */
            save_write();
            save_write();
        }
        draw();
    } else if ((s_sel == OPT_BACK && (g_pressed & KEY_A)) || (g_pressed & KEY_B)) {
        menu_save_settings();
        scene_goto(SCENE_TITLE);
    }
}
