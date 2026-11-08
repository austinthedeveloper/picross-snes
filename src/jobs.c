#include "input.h"
#include "play.h"
#include "save.h"
#include "scene.h"
#include "story.h"
#include "story_run.h"
#include "text_wrap.h"
#include "tiles.h"
#include "video.h"

/* The story hub and the jobs board (ADR-010; the web's run/index.ts and
 * jobsBoard.ts). STORY on the title goes through story_open(): an unseen
 * chapter opening plays first; then the jobs board, the path map of the
 * run in progress, or a finished run's results.
 *
 * The board lists the chapters in order. An open chapter lists its jobs,
 * and under a cleared job its Overtime and boss fights; a locked chapter
 * says which job opens it. A starts the selected job in the mode SELECT
 * picks (Overtime is always Hits on, as the web), playing its intro the
 * first time. */

enum { ROW_CHAPTER, ROW_JOB, ROW_LOCKED, ROW_EXTRA };

#define LIST_Y 3
#define LIST_ROWS 17
#define INFO_Y 21
#define MAX_ROWS 64

static u8 s_kind[MAX_ROWS];
static u8 s_value[MAX_ROWS];
static u8 s_rows;
static u8 s_sel;
static u8 s_top;

/* Goes where STORY should: an opening, the board, the map or results. */
void story_open(void)
{
    u8 c;
    u8 s;
    u8 r = run_current();

    if (r == STORY_NONE) {
        for (c = 0; c < STORY_CHAPTERS; c++) {
            s = chapter_opening(c);
            if (s != STORY_NONE && chapter_open(c) && !scene_seen(s)) {
                scene_mark_seen(s);
                save_write();
                g_story_scene = s;
                g_story_flags = 0;
                g_story_next = SCENE_JOBS;
                scene_goto(SCENE_DIALOGUE);
                return;
            }
        }
        scene_goto(SCENE_JOBS);
        return;
    }
    scene_goto(g_run.status == RUN_ACTIVE ? SCENE_MAP : SCENE_RUN_RESULT);
}

static void add_row(u8 kind, u8 value)
{
    if (s_rows < MAX_ROWS) {
        s_kind[s_rows] = kind;
        s_value[s_rows] = value;
        s_rows++;
    }
}

static void build_rows(void)
{
    u8 c;
    u8 i;
    u8 r;
    u8 x;

    s_rows = 0;
    for (c = 0; c < STORY_CHAPTERS; c++) {
        add_row(ROW_CHAPTER, c);
        if (!chapter_open(c)) {
            add_row(ROW_LOCKED, c);
            continue;
        }
        for (i = 0; i < chapter_job_count(c); i++) {
            r = chapter_job(c, i);
            add_row(ROW_JOB, r);
            if (!job_cleared(r))
                continue;
            for (x = 0; x < STORY_RUNS; x++) {
                if (run_overtime_of(x) == r || run_boss_of(x) == r)
                    add_row(ROW_EXTRA, x);
            }
        }
    }
}

static u8 selectable(u8 row)
{
    return s_kind[row] == ROW_JOB || s_kind[row] == ROW_EXTRA;
}

static void draw_row(u8 row)
{
    u8 y = (u8)(LIST_Y + row - s_top);
    u8 v = s_value[row];
    u8 x;
    u8 sel = row == s_sel;
    u8 pal = sel ? TEXT_SELECTED : TEXT_INK;

    bg3_fill(1, y, 30, 1, 0);
    bg1_fill(0, y, 32, 1, sel ? SEL_BAR : 0);
    switch (s_kind[row]) {
    case ROW_CHAPTER:
        bg3_number(1, y, chapter_order(v), 2, 0, chapter_open(v) ? TEXT_SELECTED : TEXT_DIM);
        bg3_text_n(4, y, (const char *)story_string(chapter_title(v)), 26,
            chapter_open(v) ? TEXT_INK : TEXT_DIM);
        break;
    case ROW_LOCKED:
        x = bg3_text(4, y, "Opens after ", TEXT_DIM);
        bg3_text_n(x, y,
            (const char *)story_string(run_name(run_requires(chapter_job(v, 0)))),
            (u8)(30 - x), TEXT_DIM);
        break;
    case ROW_JOB:
        bg3_text_n(4, y, (const char *)story_string(run_name(v)), 24, pal);
        if (job_cleared(v))
            bg3_text(29, y, "|", TEXT_OK);
        break;
    default:
        bg3_text(6, y, run_overtime_of(v) != STORY_NONE ? "Overtime" : "Boss:", pal);
        if (run_boss_of(v) != STORY_NONE)
            bg3_text_n(12, y, (const char *)story_string(enc_name(run_encounter(v, 0))),
                17, pal);
        if (job_cleared(v))
            bg3_text(29, y, "|", TEXT_OK);
        break;
    }
}

static void draw_list(void)
{
    u8 i;

    for (i = 0; i < LIST_ROWS; i++) {
        if (s_top + i < s_rows) {
            draw_row(s_top + i);
        } else {
            bg3_fill(1, LIST_Y + i, 30, 1, 0);
            bg1_fill(0, LIST_Y + i, 32, 1, 0);
        }
    }
}

static void draw_info(void)
{
    u8 r = s_value[s_sel];
    JobStats *j = job_stats(r);
    u8 x;

    bg3_fill(2, INFO_Y + 1, 28, 4, 0);
    text_draw_wrapped(2, INFO_Y + 1, story_string(run_blurb(r)), 28, 3, TEXT_INK);
    x = bg3_text(2, INFO_Y + 4, "", TEXT_DIM);
    bg3_number(x, INFO_Y + 4, run_length(r), 2, 0, TEXT_INK);
    x = bg3_text((u8)(x + 2), INFO_Y + 4, " STOPS  HP", TEXT_DIM);
    bg3_number(x, INFO_Y + 4, run_hp(r), 3, 0, TEXT_INK);
    if (j->hits_on_wins) {
        x = bg3_text(18, INFO_Y + 4, "BEST", TEXT_DIM);
        bg3_number(x, INFO_Y + 4, j->best_hp, 3, 0, TEXT_OK);
        bg3_text((u8)(x + 3), INFO_Y + 4, "HP", TEXT_DIM);
    } else if (j->wins) {
        x = bg3_text(18, INFO_Y + 4, "WON", TEXT_OK);
        bg3_number(x, INFO_Y + 4, j->wins, 3, 0, TEXT_OK);
    } else if (j->plays) {
        x = bg3_text(18, INFO_Y + 4, "TRIES", TEXT_DIM);
        bg3_number(x, INFO_Y + 4, j->plays, 3, 0, TEXT_INK);
    }
}

static void draw_mode(void)
{
    bg3_text(19, 27, g_save.mode == MODE_HEARTS ? "SEL:HITS ON" : "SEL:RELAXED", TEXT_DIM);
}

static void scroll_to_sel(void)
{
    if (s_sel < s_top)
        s_top = s_sel;
    if (s_sel >= s_top + LIST_ROWS)
        s_top = (u8)(s_sel - LIST_ROWS + 1);
    /* Show a selected job's chapter header when there's room. */
    if (s_top && s_top == s_sel && s_kind[s_sel - 1] == ROW_CHAPTER)
        s_top--;
}

void jobs_enter(void)
{
    u8 i;

    bg1_clear();
    bg3_clear();
    menu_header("JOBS BOARD");
    build_rows();
    if (s_sel >= s_rows || selectable(s_sel) == 0) {
        for (i = 0; i < s_rows && !selectable(i); i++)
            ;
        s_sel = i;
    }
    scroll_to_sel();
    draw_list();
    bg1_frame(1, INFO_Y, 30, 6);
    draw_info();
    bg3_text(1, 27, "A START  B BACK", TEXT_DIM);
    draw_mode();
}

static void move_sel(s8 dir)
{
    u8 old = s_sel;
    u8 top = s_top;
    u8 i = s_sel;

    do {
        i = (u8)(i + dir);
        if (i >= s_rows)
            return;
    } while (!selectable(i));
    s_sel = i;
    scroll_to_sel();
    if (top != s_top) {
        draw_list();
    } else {
        draw_row(old);
        draw_row(s_sel);
    }
    draw_info();
}

void jobs_update(void)
{
    u8 r;
    u8 mode;

    if (g_repeat & KEY_UP) {
        move_sel(-1);
    } else if (g_repeat & KEY_DOWN) {
        move_sel(1);
    } else if (g_pressed & KEY_SELECT) {
        g_save.mode = g_save.mode == MODE_HEARTS ? MODE_RELAXED : MODE_HEARTS;
        draw_mode();
        menu_settings_changed();
    } else if (g_pressed & KEY_B) {
        menu_save_settings();
        scene_goto(SCENE_TITLE);
    } else if (g_pressed & KEY_A) {
        r = s_value[s_sel];
        mode = run_overtime_of(r) != STORY_NONE ? MODE_HEARTS : g_save.mode;
        run_start(r, mode);
        g_story_next = SCENE_MAP;
        if (run_intro(r) != STORY_NONE && !scene_seen(run_intro(r))) {
            scene_mark_seen(run_intro(r));
            g_story_scene = run_intro(r);
            g_story_flags = 0;
            save_write();
            scene_goto(SCENE_DIALOGUE);
            return;
        }
        save_write();
        scene_goto(SCENE_MAP);
    }
}
