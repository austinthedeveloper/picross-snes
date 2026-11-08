#include "input.h"
#include "save.h"
#include "scene.h"
#include "story.h"
#include "story_const.h"
#include "story_play.h"
#include "story_run.h"
#include "text_wrap.h"
#include "tiles.h"
#include "video.h"

/* The path map of the run in progress (ADR-010; the web's pathMap.ts):
 * the job, HP and gadgets on top, then its encounters in order with the
 * next one to play selected, and its blurb below. A plays it (its scene
 * first, unless its board is already under way, as the web's encounter
 * screen); B goes back to the title, keeping the run. */

#define LIST_Y 5
#define LIST_ROWS 14
#define INFO_Y 21

static u8 s_top;

static u8 node_glyph(u8 e)
{
    if (enc_boss(e))
        return GLYPH_CROWN;
    if (enc_kind(e) == ENC_DIG)
        return '#';
    if (enc_kind(e) == ENC_REST)
        return GLYPH_CUP;
    return GLYPH_INVADER;
}

static void draw_gadgets(void)
{
    u8 i;
    u8 x = 1;
    u8 n = run_gadget_count_held();

    bg3_fill(1, 3, 30, 1, 0);
    if (n == 0) {
        bg3_text(1, 3, "No gadgets", TEXT_DIM);
        return;
    }
    for (i = 0; i < n && x < 28; i++) {
        x = bg3_text(x, 3, (const char *)story_string(gadget_name(g_run.gadgets[i] - 1)),
            TEXT_INK);
        x = (u8)(x + 2);
    }
}

void map_enter(void)
{
    u8 r = run_current();
    u8 i;
    u8 e;
    u8 y;
    u8 x;
    u8 len = run_length(r);

    bg1_clear();
    bg3_clear();
    menu_header((const char *)story_string(run_name(r)));
    x = bg3_text(1, 2, "HP ", TEXT_DIM);
    bg3_number(x, 2, g_run.hp, 2, 0, TEXT_OK);
    bg3_text((u8)(x + 2), 2, "/", TEXT_DIM);
    bg3_number((u8)(x + 3), 2, g_run.max_hp, 2, 0, TEXT_INK);
    bg3_text(20, 2, g_run.mode == 1 ? "HITS ON" : "RELAXED", TEXT_DIM);
    draw_gadgets();
    if (g_run.node >= s_top + LIST_ROWS || g_run.node < s_top)
        s_top = g_run.node > 2 ? (u8)(g_run.node - 2) : 0;
    for (i = 0; i < LIST_ROWS && s_top + i < len; i++) {
        e = run_encounter(r, s_top + i);
        y = (u8)(LIST_Y + i);
        if (s_top + i < g_run.node) {
            bg3_text(2, y, "|", TEXT_OK);
            bg3_put(4, y, BG_ENTRY(node_glyph(e) - 32, TEXT_DIM) | BG_PRIO);
            bg3_text_n(6, y, (const char *)story_string(enc_name(e)), 24, TEXT_DIM);
        } else if (s_top + i == g_run.node) {
            bg1_fill(0, y, 32, 1, SEL_BAR);
            menu_pointer(1, y, 1);
            bg3_put(4, y, BG_ENTRY(node_glyph(e) - 32, TEXT_SELECTED) | BG_PRIO);
            bg3_text_n(6, y, (const char *)story_string(enc_name(e)), 24, TEXT_SELECTED);
        } else {
            bg3_put(4, y, BG_ENTRY(node_glyph(e) - 32, TEXT_INK) | BG_PRIO);
            bg3_text_n(6, y, (const char *)story_string(enc_name(e)), 24, TEXT_INK);
        }
    }
    bg1_frame(1, INFO_Y, 30, 6);
    e = run_encounter(r, g_run.node);
    text_draw_wrapped(2, INFO_Y + 1, story_string(enc_blurb(e)), 28, 4, TEXT_INK);
    bg3_text(1, 27, "A PLAY  B TITLE", TEXT_DIM);
}

void map_update(void)
{
    u8 r;
    u8 e;

    if (g_pressed & KEY_B) {
        scene_goto(SCENE_TITLE);
        return;
    }
    if ((g_pressed & KEY_A) == 0)
        return;
    r = run_current();
    e = run_encounter(r, g_run.node);
    g_play_story = 1;
    g_return_scene = SCENE_MAP;
    if (enc_scene(e) != STORY_NONE && !g_run.board_on && !g_run.battle_on) {
        g_story_scene = enc_scene(e);
        g_story_flags = 0;
        g_story_next = SCENE_PLAY;
        scene_goto(SCENE_DIALOGUE);
        return;
    }
    scene_goto(SCENE_PLAY);
}
