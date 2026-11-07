#include "board.h"
#include "board_view.h"
#include "play.h"
#include "tiles.h"
#include "video.h"

u8 g_view_bx;
u8 g_view_by;
u8 g_view_lines_done;

/* What's on screen now, to redraw only what changed. */
static u8 s_cur_x;
static u8 s_cur_y;
static u8 s_row_done[MAX_SIDE];
static u8 s_col_done[MAX_SIDE];
static u16 s_shown_seconds;
static u8 s_shown_hearts;
static u8 s_shown_hints;
static u8 s_picture;

/* Column c's guide flag (every fifth line), without a modulo. */
static const u8 s_guide[MAX_SIDE] = {
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0
};

static u16 cell_entry(u8 r, u8 c)
{
    u8 cell = g_cells[g_row_start[r] + c];
    u8 hl = (r == g_play.cur_y || c == g_play.cur_x) ? 1 : 0;
    u8 state = cell;
    u8 pal;
    u16 tile;

    if (s_picture) {
        return BG_ENTRY(cell == CELL_FILLED ? T_PIC_FULL : T_PIC_EMPTY,
            PAL_NORMAL);
    }
    if (cell == CELL_MISTAKE) {
        state = CELL_MARKED;
        pal = hl ? PAL_MISTAKE_HL : PAL_MISTAKE;
    } else {
        pal = hl ? PAL_HL : PAL_NORMAL;
    }
    if (r == g_puzzle.height - 1)
        tile = T_CELL_B + (state << 1) + s_guide[c];
    else
        tile = T_CELL + (state << 2) + s_guide[c] + (s_guide[r] << 1);
    return BG_ENTRY(tile, pal);
}

static void draw_cell(u8 r, u8 c)
{
    bg1_put((u8)(g_view_bx + c), (u8)(g_view_by + r), cell_entry(r, c));
}

static u8 clue_pal(u8 done, u8 hl)
{
    if (done)
        return hl ? PAL_DONE_HL : PAL_DONE;
    return hl ? PAL_HL : PAL_NORMAL;
}

static void draw_row_clues(u8 r)
{
    u8 count = g_row_clue_count[r];
    u8 pal = clue_pal(s_row_done[r], r == g_play.cur_y);
    u8 y = (u8)(g_view_by + r);
    u8 x = (u8)(g_view_bx - g_max_row_clues);
    const u8 *clues = g_row_clues + ((u16)r << 4);
    u8 k;
    u16 blank = (r & 1) ? T_CLUE_BLANK_ALT : T_CLUE_BLANK;
    u16 base = (r & 1) ? T_CLUE_ALT : T_CLUE;

    for (k = 0; k < g_max_row_clues - count; k++)
        bg1_put(x++, y, BG_ENTRY(blank, pal));
    for (k = 0; k < count; k++)
        bg1_put(x++, y, BG_ENTRY(base + clues[k], pal));
}

static void draw_col_clues(u8 c)
{
    u8 count = g_col_clue_count[c];
    u8 pal = clue_pal(s_col_done[c], c == g_play.cur_x);
    u8 x = (u8)(g_view_bx + c);
    u8 y = (u8)(g_view_by - g_max_col_clues);
    const u8 *clues = g_col_clues + ((u16)c << 4);
    u8 k;
    u16 blank = (c & 1) ? T_CLUE_BLANK_ALT : T_CLUE_BLANK;
    u16 base = (c & 1) ? T_CLUE_ALT : T_CLUE;

    for (k = 0; k < g_max_col_clues - count; k++)
        bg1_put(x, y++, BG_ENTRY(blank, pal));
    for (k = 0; k < count; k++)
        bg1_put(x, y++, BG_ENTRY(base + clues[k], pal));
}

static void draw_row(u8 r)
{
    u8 c;

    for (c = 0; c < g_puzzle.width; c++)
        draw_cell(r, c);
    draw_row_clues(r);
}

/* A row's (column's) cells and clue band, highlighted or not. */
static void highlight_row(u8 r, u8 on)
{
    bg1_highlight((u8)(g_view_bx - g_max_row_clues), (u8)(g_view_by + r),
        (u8)(g_max_row_clues + g_puzzle.width), 0, on);
}

static void highlight_col(u8 c, u8 on)
{
    bg1_highlight((u8)(g_view_bx + c), (u8)(g_view_by - g_max_col_clues),
        (u8)(g_max_col_clues + g_puzzle.height), 1, on);
}

static void layout(void)
{
    switch (g_puzzle.width) {
    case 5:
        g_view_bx = 15;
        g_view_by = 12;
        break;
    case 10:
        g_view_bx = 14;
        g_view_by = 11;
        break;
    case 15:
        g_view_bx = 13;
        g_view_by = 10;
        break;
    default:
        g_view_bx = 11;
        g_view_by = 8;
        break;
    }
}

void view_cursor(u8 on)
{
    if (on == 0 || s_picture) {
        cursor_hide();
        return;
    }
    cursor_show((u16)((g_view_bx + g_play.cur_x) << 3) - 1,
        (u16)((g_view_by + g_play.cur_y) << 3) - 2);
}

void view_draw_hud(void)
{
    u8 i;

    {
        static const char *const sizes[4] = { "5X5", "10X10", "15X15", "20X20" };
        u8 s = g_puzzle.width == 5 ? 0 : g_puzzle.width == 10 ? 1 :
            g_puzzle.width == 15 ? 2 : 3;

        bg3_text(1, HUD_SIZE_ROW, sizes[s], TEXT_DIM);
    }
    bg3_text(1, HUD_TIME_ROW, "TIME", TEXT_DIM);
    bg3_time(6, HUD_TIME_ROW, g_play.seconds, TEXT_INK);
    s_shown_seconds = g_play.seconds;
    if (g_play.mode == MODE_HEARTS) {
        for (i = 0; i < MAX_HEARTS; i++)
            bg3_text((u8)(1 + i), HUD_HEARTS_ROW,
                i < g_play.hearts ? "{" : "}", TEXT_INK);
    } else {
        bg3_text(1, HUD_HEARTS_ROW, "RELAXED", TEXT_DIM);
    }
    s_shown_hearts = g_play.hearts;
    bg3_text(1, HUD_HINTS_ROW, "HINTS", TEXT_DIM);
    bg3_number(7, HUD_HINTS_ROW, MAX_HINTS - g_play.hints_used, 1, 0,
        g_play.hints_used < MAX_HINTS ? TEXT_INK : TEXT_DIM);
    s_shown_hints = g_play.hints_used;
}

void view_clear_corner(u8 from, u8 to)
{
    /* BG3 is free left of the board and above it (clues are on BG1). */
    if (from < to)
        bg3_fill(0, from, g_view_bx, (u8)(to - from), 0);
}

void view_draw_all(void)
{
    u8 r;
    u8 c;

    layout();
    s_picture = 0;
    bg1_clear();
    bg3_clear();
    for (r = 0; r < g_puzzle.height; r++)
        s_row_done[r] = board_row_done(r);
    for (c = 0; c < g_puzzle.width; c++)
        s_col_done[c] = board_col_done(c);
    for (r = 0; r < g_puzzle.height; r++) {
        draw_row(r);
        bg1_put((u8)(g_view_bx + g_puzzle.width), (u8)(g_view_by + r),
            BG_ENTRY(r == g_puzzle.height - 1 ? T_EDGE_RB : T_EDGE_R,
                PAL_NORMAL));
    }
    for (c = 0; c < g_puzzle.width; c++)
        draw_col_clues(c);
    s_cur_x = g_play.cur_x;
    s_cur_y = g_play.cur_y;
    g_board_dirty_count = 0;
    g_board_dirty_all = 0;
    view_draw_hud();
    view_cursor(1);
}

void view_draw_picture(void)
{
    u8 r;
    u8 c;
    u8 i;

    s_picture = 1;
    for (r = 0; r < g_puzzle.height; r++) {
        for (c = 0; c < g_puzzle.width; c++)
            draw_cell(r, c);
        for (i = 0; i < g_max_row_clues; i++)
            bg1_put((u8)(g_view_bx - 1 - i), (u8)(g_view_by + r), 0);
        bg1_put((u8)(g_view_bx + g_puzzle.width), (u8)(g_view_by + r), 0);
    }
    for (c = 0; c < g_puzzle.width; c++)
        for (i = 0; i < g_max_col_clues; i++)
            bg1_put((u8)(g_view_bx + c), (u8)(g_view_by - 1 - i), 0);
    cursor_hide();
}

static void refresh_lines(u8 r, u8 c)
{
    u8 done = board_row_done(r);

    if (done != s_row_done[r]) {
        s_row_done[r] = done;
        g_view_lines_done += done;
        draw_row_clues(r);
    }
    done = board_col_done(c);
    if (done != s_col_done[c]) {
        s_col_done[c] = done;
        g_view_lines_done += done;
        draw_col_clues(c);
    }
}

void view_update(void)
{
    u8 i;
    u16 index;
    u8 r;
    u8 c;

    if (s_picture)
        return;
    if (g_board_dirty_all) {
        g_board_dirty_all = 0;
        g_board_dirty_count = 0;
        for (r = 0; r < g_puzzle.height; r++) {
            s_row_done[r] = board_row_done(r);
            draw_row(r);
        }
        for (c = 0; c < g_puzzle.width; c++) {
            s_col_done[c] = board_col_done(c);
            draw_col_clues(c);
        }
    }
    for (i = 0; i < g_board_dirty_count; i++) {
        index = g_board_dirty[i];
        /* Row and column from the index, by the row start table. */
        r = 0;
        while (r + 1 < g_puzzle.height && g_row_start[r + 1] <= index)
            r++;
        c = (u8)(index - g_row_start[r]);
        draw_cell(r, c);
        refresh_lines(r, c);
    }
    g_board_dirty_count = 0;
    if (s_cur_x != g_play.cur_x || s_cur_y != g_play.cur_y) {
        /* Only the highlight bit changes: clear the old lines, then set
         * the new ones (so cells on both keep it). */
        if (s_cur_y != g_play.cur_y)
            highlight_row(s_cur_y, 0);
        if (s_cur_x != g_play.cur_x)
            highlight_col(s_cur_x, 0);
        highlight_row(g_play.cur_y, 1);
        highlight_col(g_play.cur_x, 1);
        s_cur_x = g_play.cur_x;
        s_cur_y = g_play.cur_y;
        view_cursor(1);
    }
    if (s_shown_seconds != g_play.seconds) {
        s_shown_seconds = g_play.seconds;
        bg3_time(6, HUD_TIME_ROW, g_play.seconds, TEXT_INK);
    }
    if (s_shown_hearts != g_play.hearts || s_shown_hints != g_play.hints_used)
        view_draw_hud();
}
