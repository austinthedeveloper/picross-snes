#include "board.h"
#include "board_view.h"
#include "input.h"
#include "play.h"
#include "progress.h"
#include "scene.h"
#include "stats.h"
#include "video.h"

/* The play screen (docs/CONTROLS.md):
 *   d-pad move (repeats; wraps at the edges)   A fill / erase (hold to paint)
 *   B mark / unmark (hold to paint)            L undo   R redo
 *   X hint menu                                START pause */
enum { PS_PLAY, PS_PAUSE, PS_HINT, PS_SOLVED, PS_FAILED };
enum { PAUSE_RESUME, PAUSE_RESTART, PAUSE_QUIT, PAUSE_COUNT };

static u8 s_state;
static u8 s_sel;
static u16 s_stroke_button;
/* Frames since the puzzle was solved (the picture shows first). */
static u8 s_timer;

/* The stats before this puzzle, to tell what a solve earned. */
static u8 s_rank_before;
static u8 s_tiers_before[COLLECTION_COUNT];
static u8 s_ach_before[(ACHIEVEMENT_COUNT + 7) >> 3];

static const char *const s_pause_items[PAUSE_COUNT] = {
    "RESUME", "RESTART", "QUIT"
};
static const char *const s_hint_items[3] = { "CELL", "ROW", "COLUMN" };

static void snapshot_stats(void)
{
    u8 i;

    stats_compute(&g_stats);
    s_rank_before = g_stats.rank;
    for (i = 0; i < COLLECTION_COUNT; i++)
        s_tiers_before[i] = g_stats.tiers[i];
    for (i = 0; i < sizeof s_ach_before; i++)
        s_ach_before[i] = 0;
    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
        if (achievement_done(i, &g_stats))
            s_ach_before[i >> 3] |= (u8)(1 << (i & 7));
}

static void add_news(u8 kind, u8 id, u8 value)
{
    if (g_news_count >= NEWS_MAX)
        return;
    g_news_kind[g_news_count] = kind;
    g_news_id[g_news_count] = id;
    g_news_value[g_news_count] = value;
    g_news_count++;
}

/* Frames the last solve's bookkeeping took (probed by smoke runs). */
u16 g_solve_frames;

/* Records the solve, saves, and works out what it earned. */
static void finish_solve(void)
{
    u8 i;
    u8 c;
    u16 start = snes_vblank_count;

    g_solve_result = progress_record_solve();
    save_write();
    stats_compute(&g_stats);
    g_news_count = 0;
    if (g_stats.rank != s_rank_before)
        add_news(NEWS_RANK, g_stats.rank, 0);
    c = g_puzzle.slot < PUZZLE_COUNT ? pack_collection(g_puzzle.slot) : 0;
    if (g_stats.tiers[c] != s_tiers_before[c])
        add_news(NEWS_BADGE, c, g_stats.tiers[c]);
    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
        if ((s_ach_before[i >> 3] & (1 << (i & 7))) == 0 &&
            achievement_done(i, &g_stats))
            add_news(NEWS_ACHIEVEMENT, i, 0);
    g_solve_frames = snes_vblank_count - start;
}

static void draw_menu(const char *const *items, u8 count, u8 x, u8 y)
{
    u8 i;

    for (i = 0; i < count; i++) {
        menu_pointer((u8)(x - 2), (u8)(y + i), i == s_sel);
        bg3_text(x, (u8)(y + i), items[i], i == s_sel ? TEXT_SELECTED : TEXT_INK);
    }
}

static void open_pause(void)
{
    s_state = PS_PAUSE;
    s_sel = PAUSE_RESUME;
    bgSetDisable(0);
    view_cursor(0);
    bg3_center(10, "PAUSED", TEXT_INK);
    draw_menu(s_pause_items, PAUSE_COUNT, 13, 13);
}

static void close_pause(void)
{
    bg3_fill(0, 10, 32, 6, 0);
    view_draw_hud();
    bgSetEnable(0);
    view_cursor(1);
    s_state = PS_PLAY;
}

static void quit_to_list(void)
{
    if (g_play.solved == 0 && g_play.failed == 0) {
        u16 i;
        u8 started = 0;

        for (i = 0; i < g_puzzle.cells; i++)
            if (g_cells[i] != CELL_EMPTY)
                started = 1;
        /* Keep a started board to come back to (one at a time). */
        if (started || g_play.seconds >= 10)
            progress_suspend();
        save_write();
    }
    scene_goto(SCENE_PUZZLES);
}

static void open_hint(void)
{
    if (g_play.hints_used >= MAX_HINTS)
        return;
    s_state = PS_HINT;
    s_sel = 0;
    bg3_text(1, HUD_MENU_ROW, "HINT:", TEXT_SELECTED);
    draw_menu(s_hint_items, 3, 3, HUD_MENU_ROW + 1);
}

static void close_hint(void)
{
    view_clear_corner(HUD_MENU_ROW, HUD_MENU_ROW + 4);
    s_state = PS_PLAY;
}

static void show_failed(void)
{
    s_state = PS_FAILED;
    view_cursor(0);
    bg3_text(1, HUD_MENU_ROW, "NO HEARTS", TEXT_SELECTED);
    bg3_text(1, HUD_MENU_ROW + 1, "A RETRY", TEXT_INK);
    bg3_text(1, HUD_MENU_ROW + 2, "B QUIT", TEXT_INK);
}

static void show_solved(void)
{
    s_state = PS_SOLVED;
    s_timer = 0;
    play_stroke_end();
    view_update();
    view_draw_picture();
    view_clear_corner(0, g_view_by);
    bg3_text(1, 1, "SOLVED!", TEXT_SELECTED);
    /* Show the picture before the bookkeeping (about 7 frames). */
    video_frame();
    video_frame();
    finish_solve();
}

static void move_cursor(void)
{
    u8 moved = 0;

    if (g_repeat & KEY_LEFT) {
        g_play.cur_x = g_play.cur_x ? (u8)(g_play.cur_x - 1) : (u8)(g_puzzle.width - 1);
        moved = 1;
    } else if (g_repeat & KEY_RIGHT) {
        g_play.cur_x = (u8)(g_play.cur_x + 1) < g_puzzle.width ? (u8)(g_play.cur_x + 1) : 0;
        moved = 1;
    }
    if (g_repeat & KEY_UP) {
        g_play.cur_y = g_play.cur_y ? (u8)(g_play.cur_y - 1) : (u8)(g_puzzle.height - 1);
        moved = 1;
    } else if (g_repeat & KEY_DOWN) {
        g_play.cur_y = (u8)(g_play.cur_y + 1) < g_puzzle.height ? (u8)(g_play.cur_y + 1) : 0;
        moved = 1;
    }
    if (moved && s_stroke_button)
        play_stroke_continue();
}

static void update_play(void)
{
    if (g_pressed & KEY_START) {
        play_stroke_end();
        s_stroke_button = 0;
        open_pause();
        return;
    }
    /* A stroke ends when its button is let go. */
    if (s_stroke_button && (g_held & s_stroke_button) == 0) {
        play_stroke_end();
        s_stroke_button = 0;
    }
    if (s_stroke_button == 0) {
        if (g_pressed & KEY_A) {
            s_stroke_button = KEY_A;
            play_stroke_begin(1);
        } else if (g_pressed & KEY_B) {
            s_stroke_button = KEY_B;
            play_stroke_begin(0);
        } else if (g_pressed & KEY_L) {
            play_undo();
        } else if (g_pressed & KEY_R) {
            play_redo();
        } else if (g_pressed & KEY_X) {
            open_hint();
            return;
        }
    }
    move_cursor();
    play_tick();
    view_update();
    if (g_play.solved)
        show_solved();
    else if (g_play.failed)
        show_failed();
}

void play_scene_enter(void)
{
    snapshot_stats();
    if (g_play_resume == 0 || progress_resume(g_play_slot) == 0)
        play_start(g_play_slot, g_save.mode);
    g_play_resume = 0;
    s_state = PS_PLAY;
    s_sel = 0;
    s_stroke_button = 0;
    s_timer = 0;
    view_draw_all();
    bgSetEnable(0);
}

void play_scene_update(void)
{
    switch (s_state) {
    case PS_PLAY:
        update_play();
        break;
    case PS_PAUSE:
        if (menu_move(&s_sel, PAUSE_COUNT))
            draw_menu(s_pause_items, PAUSE_COUNT, 13, 13);
        if (g_pressed & (KEY_B | KEY_START)) {
            close_pause();
        } else if (g_pressed & KEY_A) {
            if (s_sel == PAUSE_RESUME) {
                close_pause();
            } else if (s_sel == PAUSE_RESTART) {
                play_restart();
                close_pause();
                view_draw_all();
            } else {
                bgSetEnable(0);
                quit_to_list();
            }
            input_swallow();
        }
        break;
    case PS_HINT:
        if (menu_move(&s_sel, 3))
            draw_menu(s_hint_items, 3, 3, HUD_MENU_ROW + 1);
        if (g_pressed & (KEY_B | KEY_X)) {
            close_hint();
        } else if (g_pressed & KEY_A) {
            play_hint(s_sel);
            close_hint();
            input_swallow();
            view_update();
            if (g_play.solved)
                show_solved();
        }
        break;
    case PS_FAILED:
        if (g_pressed & KEY_A) {
            play_restart();
            s_state = PS_PLAY;
            view_draw_all();
            input_swallow();
        } else if (g_pressed & KEY_B) {
            if (g_save.suspend.slot1 == g_puzzle.slot + 1)
                progress_clear_suspend();
            save_write();
            scene_goto(SCENE_PUZZLES);
        }
        break;
    case PS_SOLVED:
        if (s_timer < 255)
            s_timer++;
        if (s_timer > 30 && (g_pressed & (KEY_A | KEY_B | KEY_START)))
            scene_goto(SCENE_RESULT);
        break;
    }
}
