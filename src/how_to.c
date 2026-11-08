#include "input.h"
#include "scene.h"
#include "video.h"

static const char *const s_lines[] = {
    "FILL CELLS SO EACH ROW AND",
    "COLUMN MATCHES ITS CLUES: THE",
    "NUMBERS ARE THE RUNS OF FILLED",
    "CELLS, IN ORDER. A PICTURE",
    "APPEARS WHEN YOU'RE DONE.",
    "",
    "D-PAD   MOVE",
    "A       FILL / ERASE",
    "B       MARK X / UNMARK",
    "        HOLD A OR B TO PAINT",
    "L  R    UNDO  REDO",
    "X       HINT (3 A PUZZLE)",
    "START   PAUSE",
    "SELECT  MODE (ON THE LISTS)",
    "",
    "HEARTS MODE CHECKS EVERY FILL.",
    "SOLVE WITH NO MISTAKES AND NO",
    "HINTS FOR A PERFECT CLEAR.",
    0
};

void how_to_enter(void)
{
    u8 i;

    bg1_clear();
    bg3_clear();
    menu_header("HOW TO PLAY");
    for (i = 0; s_lines[i]; i++)
        bg3_text(1, (u8)(4 + i), s_lines[i], i >= 6 && i <= 13 ? TEXT_INK : TEXT_DIM);
    menu_footer("B BACK");
}

void how_to_update(void)
{
    if (g_pressed & (KEY_A | KEY_B))
        scene_goto(SCENE_TITLE);
}
