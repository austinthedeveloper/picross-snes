#ifndef SCENE_H
#define SCENE_H

#include "types.h"

enum {
    SCENE_TITLE = 0,
    SCENE_COLLECTIONS,
    SCENE_PUZZLES,
    SCENE_PLAY,
    SCENE_RESULT,
    SCENE_RECORDS,
    SCENE_BADGES,
    SCENE_ACHIEVEMENTS,
    SCENE_OPTIONS,
    SCENE_HOW_TO,
    SCENE_MURALS,
    SCENE_DIALOGUE,
    SCENE_JOBS,
    SCENE_MAP,
    SCENE_RUN_RESULT,
    SCENE_COUNT,
    SCENE_NONE = 0xFF
};

/* Asks for a scene change at the end of this frame (with a fade). */
void scene_goto(u8 scene);

/* What the menus remember between screens. */
extern u8 g_sel_collection;
extern u8 g_sel_index;
extern u8 g_sel_mural;
/* Where the play and result screens go back to (puzzles or murals). */
extern u8 g_return_scene;
/* The slot to play, and whether to resume its suspended board. */
extern u16 g_play_slot;
extern u8 g_play_resume;

/* News from the last solve, for the result screen. */
enum { NEWS_RANK = 1, NEWS_BADGE = 2, NEWS_ACHIEVEMENT = 3 };
#define NEWS_MAX 4
extern u8 g_news_count;
extern u8 g_news_kind[NEWS_MAX];
extern u8 g_news_id[NEWS_MAX];
extern u8 g_news_value[NEWS_MAX];
extern u8 g_solve_result;

/* Each scene: enter (in forced blank, draws everything) and update. */
void title_enter(void);
void title_update(void);
void collections_enter(void);
void collections_update(void);
void puzzles_enter(void);
void puzzles_update(void);
void play_scene_enter(void);
void play_scene_update(void);
void result_enter(void);
void result_update(void);
void records_enter(void);
void records_update(void);
void badges_enter(void);
void badges_update(void);
void achievements_enter(void);
void achievements_update(void);
void options_enter(void);
void options_update(void);
void how_to_enter(void);
void how_to_update(void);
void murals_enter(void);
void murals_update(void);
void murals_init(void);
/* A story scene (scene_view.c): plays g_story_scene with g_story_flags,
 * then goes to g_story_next. */
extern u8 g_story_scene;
extern u8 g_story_flags;
extern u8 g_story_next;
void dialogue_enter(void);
void dialogue_update(void);
void dialogue_leave(void);
/* Story mode (jobs.c, map.c, run_result.c). */
void story_open(void);
void jobs_enter(void);
void jobs_update(void);
void map_enter(void);
void map_update(void);
void run_result_enter(void);
void run_result_update(void);
/* The next unsolved tile of g_sel_mural (selected), or 0xFF. */
u8 murals_next_tile(void);
/* Opens the murals screen on this tile of g_sel_mural. */
void murals_select_tile(u8 tile);

/* The selected row's highlighter band (BG1). */
#define SEL_BAR BG_ENTRY(T_CLUE_BLANK, PAL_HL)

/* Shared menu drawing (menus.c). */
void menu_header(const char *title);
void menu_footer(const char *text);
/* Moves *sel by the d-pad's up/down (repeating), wrapping in 0..count-1.
 * Returns 1 when it moved. */
u8 menu_move(u8 *sel, u8 count);
void menu_pointer(u8 x, u8 y, u8 on);
/* The mode toggle (SELECT on the lists). */
void menu_draw_mode(u8 y);
u8 menu_mode_toggle(void);
/* Settings changed on a menu are saved when it closes. */
void menu_settings_changed(void);
void menu_save_settings(void);
void menu_init(void);

#endif
