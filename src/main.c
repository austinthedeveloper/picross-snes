#include "input.h"
#include "save.h"
#include "scene.h"
#include "video.h"

u8 g_sel_collection;
u8 g_sel_index;
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
    }
}

int main(void)
{
    consoleInit();
    video_init();
    save_load();
    /* WRAM is not cleared at power-on: every global the scenes share. */
    menu_init();
    badges_init();
    achievements_init();
    g_title_sel = 0;
    g_sel_collection = 0;
    g_sel_index = 0;
    g_play_slot = 0;
    g_play_resume = 0;
    g_news_count = 0;
    g_solve_result = 0;
    s_next = SCENE_NONE;
    s_scene = SCENE_TITLE;
    enter(s_scene);
    input_swallow();
    video_fade_in();
    while (1) {
        input_update();
        update(s_scene);
        if (s_next != SCENE_NONE) {
            video_fade_out();
            s_scene = s_next;
            s_next = SCENE_NONE;
            cursor_hide();
            enter(s_scene);
            input_swallow();
            video_fade_in();
            continue;
        }
        video_frame();
    }
    return 0;
}
