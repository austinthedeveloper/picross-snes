#include "dialogue.h"
#include "input.h"
#include "save.h"
#include "scene.h"
#include "story.h"
#include "story_run.h"
#include "text_wrap.h"
#include "video.h"

/* A finished run (ADR-010; the web's results.ts): its result is recorded
 * once, then "Job done!" or "Run over", encounters cleared and HP left,
 * and every line the job's outro plays for that outcome (the sign-off).
 * A runs it again; B goes back to the jobs board. */

void run_result_enter(void)
{
    u8 r = run_current();
    u8 won = g_run.status == RUN_WON;
    u8 x;
    u8 y = 7;
    Dialogue d;
    Step s;
    const u8 *scene;

    run_record_result();
    save_write();
    bg1_clear();
    bg3_clear();
    menu_header((const char *)story_string(run_name(r)));
    bg3_center(3, won ? "JOB DONE!" : "RUN OVER", won ? TEXT_OK : TEXT_SELECTED);
    x = bg3_text(4, 5, "", TEXT_INK);
    bg3_number(x, 5, g_run.node < run_length(r) ? g_run.node : run_length(r), 2, 0, TEXT_INK);
    bg3_text((u8)(x + 2), 5, "/", TEXT_DIM);
    bg3_number((u8)(x + 3), 5, run_length(r), 2, 0, TEXT_INK);
    x = bg3_text((u8)(x + 5), 5, " CLEARED  ", TEXT_DIM);
    bg3_number(x, 5, g_run.hp, 2, 0, TEXT_OK);
    bg3_text((u8)(x + 2), 5, " HP LEFT", TEXT_DIM);
    if (run_outro(r) != STORY_NONE) {
        scene = story_scene(run_outro(r));
        dialogue_start(&d, run_outro(r), won ? 1 << FLAG_WON : 0);
        for (dialogue_step(&d, &s); s.kind == STEP_LINE && y < 24; dialogue_step(&d, &s)) {
            if (s.who != STORY_NONE) {
                bg3_text(2, y, (const char *)story_string(scene_cast_name(scene, s.who)),
                    TEXT_SELECTED);
                y++;
            }
            y = (u8)(y + text_draw_wrapped(2, y, story_string(s.text), 28, 5, TEXT_INK) + 1);
            dialogue_advance(&d);
        }
    }
    bg3_text(1, 27, "A RUN IT AGAIN  B JOBS BOARD", TEXT_DIM);
}

void run_result_update(void)
{
    u8 r = run_current();

    if (g_pressed & KEY_A) {
        run_start(r, g_run.mode);
        save_write();
        scene_goto(SCENE_MAP);
    } else if (g_pressed & KEY_B) {
        run_clear();
        save_write();
        scene_goto(SCENE_JOBS);
    }
}
