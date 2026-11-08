#include "audio.h"
#include "board.h"
#include "board_view.h"
#include "input.h"
#include "save.h"
#include "art.h"
#include "scene.h"
#include "story.h"
#include "video.h"

u8 g_sel_collection;
u8 g_sel_index;
u8 g_sel_mural;
u8 g_return_scene;
u16 g_play_slot;
u8 g_play_resume;
u8 g_news_count;
u8 g_news_kind[NEWS_MAX];
u8 g_news_id[NEWS_MAX];
u8 g_news_value[NEWS_MAX];
u8 g_solve_result;

extern u8 g_title_sel;
extern void badges_init(void);
extern void achievements_init(void);
static u8 s_scene;
static u8 s_next;

void scene_goto(u8 scene)
{
    s_next = scene;
}

/* Menu sounds on every screen but the board, which plays its own. */
static void menu_sounds(void)
{
    if (s_scene == SCENE_PLAY)
        return;
    if (g_pressed & KEY_B)
        audio_play(SFX_BACK, 9);
    else if (g_pressed & (KEY_A | KEY_START))
        audio_play(SFX_CONFIRM, 9);
    else if (g_repeat & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_SELECT))
        audio_play(SFX_BLIP, 7);
}

static void enter(u8 scene)
{
    switch (scene) {
    case SCENE_TITLE: title_enter(); break;
    case SCENE_COLLECTIONS: collections_enter(); break;
    case SCENE_PUZZLES: puzzles_enter(); break;
    case SCENE_PLAY: play_scene_enter(); break;
    case SCENE_RESULT: result_enter(); break;
    case SCENE_RECORDS: records_enter(); break;
    case SCENE_BADGES: badges_enter(); break;
    case SCENE_ACHIEVEMENTS: achievements_enter(); break;
    case SCENE_OPTIONS: options_enter(); break;
    case SCENE_HOW_TO: how_to_enter(); break;
    case SCENE_MURALS: murals_enter(); break;
    case SCENE_DIALOGUE: dialogue_enter(); break;
    }
}

static void update(u8 scene)
{
    switch (scene) {
    case SCENE_TITLE: title_update(); break;
    case SCENE_COLLECTIONS: collections_update(); break;
    case SCENE_PUZZLES: puzzles_update(); break;
    case SCENE_PLAY: play_scene_update(); break;
    case SCENE_RESULT: result_update(); break;
    case SCENE_RECORDS: records_update(); break;
    case SCENE_BADGES: badges_update(); break;
    case SCENE_ACHIEVEMENTS: achievements_update(); break;
    case SCENE_OPTIONS: options_update(); break;
    case SCENE_HOW_TO: how_to_update(); break;
    case SCENE_MURALS: murals_update(); break;
    case SCENE_DIALOGUE: dialogue_update(); break;
    }
}

int main(void)
{
    consoleInit();
    video_init();
    save_load();
    audio_init();
    /* WRAM is not cleared at power-on: every global the scenes share. */
    menu_init();
    badges_init();
    achievements_init();
    g_title_sel = 0;
    g_sel_collection = 0;
    g_sel_index = 0;
    g_sel_mural = 0;
    g_return_scene = SCENE_PUZZLES;
    murals_init();
    story_init();
    art_init();
    g_story_scene = 0;
    g_story_flags = 0;
    g_story_next = SCENE_TITLE;
    g_play_slot = 0;
    g_play_resume = 0;
    g_news_count = 0;
    g_board_changes = 0;
    g_board_last_value = 0;
    g_view_lines_done = 0;
    g_solve_result = 0;
    s_next = SCENE_NONE;
    s_scene = SCENE_TITLE;
    enter(s_scene);
    input_swallow();
    video_fade_in();
    while (1) {
        input_update();
        menu_sounds();
        update(s_scene);
        audio_update();
        if (s_next != SCENE_NONE) {
            video_fade_out();
            if (s_scene == SCENE_DIALOGUE)
                dialogue_leave();
            s_scene = s_next;
            s_next = SCENE_NONE;
            cursor_hide();
            enter(s_scene);
            input_swallow();
            video_fade_in();
            audio_update();
            continue;
        }
        video_frame();
    }
    return 0;
}
