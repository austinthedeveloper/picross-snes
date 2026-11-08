#include "board.h"
#include "board_view.h"
#include "play.h"
#include "tiles.h"
#include "video.h"
#include "big_tiles.h"

u8 g_view_bx;
u8 g_view_by;
u8 g_view_lines_done;
/* Story fights (ADR-010) change what the board shows; NULL: free play. */
u8 (*g_view_clue_hook)(u8 id, u8 *out);
u8 (*g_view_done_hook)(u8 id);
u8 (*g_view_ink_hook)(u16 index);
void (*g_view_hud_hook)(void);
u8 g_view_new_lines[VIEW_NEW_MAX];
u8 g_view_new_count;
static u8 s_shown[16];

/* What's on screen now, to redraw only what changed. */
static u8 s_cur_x;
static u8 s_cur_y;
static u8 s_row_done[MAX_SIDE];
static u8 s_col_done[MAX_SIDE];
static u16 s_shown_seconds;
static u8 s_shown_hearts;
static u8 s_shown_hints;
static u8 s_picture;
/* 1 for 16-pixel cells and clues (5x5 and 10x10; docs/LAYOUT.md): every
 * cell and clue is then 2x2 tiles, and positions shift left by one. */
static u8 s_big;

/* Column c's guide flag (every fifth line), without a modulo. */
static const u8 s_guide[MAX_SIDE] = {
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0
};

/* A cell's top-left tile on screen. */
static u8 cell_x(u8 c)
{
    return (u8)(g_view_bx + (c << s_big));
}

static u8 cell_y(u8 r)
{
    return (u8)(g_view_by + (r << s_big));
}

static void put_big(u8 x, u8 y, const u16 *tiles, u16 attr)
{
    bg1_put(x, y, tiles[0] | attr);
    bg1_put((u8)(x + 1), y, tiles[1] | attr);
    bg1_put(x, (u8)(y + 1), tiles[2] | attr);
    bg1_put((u8)(x + 1), (u8)(y + 1), tiles[3] | attr);
}

static void draw_cell(u8 r, u8 c)
{
    u8 cell = g_cells[g_row_start[r] + c];
    u8 hl = (r == g_play.cur_y || c == g_play.cur_x) ? 1 : 0;
    u8 state = cell;
    u8 pal;
    u8 bottom = r == g_puzzle.height - 1;
    u16 tile;
    u8 x = cell_x(c);
    u8 y = cell_y(r);

    thumb_set(c, r, cell == CELL_FILLED);
    if (s_picture) {
        tile = cell == CELL_FILLED ? T_PIC_FULL : T_PIC_EMPTY;
        bg1_put(x, y, BG_ENTRY(tile, PAL_NORMAL));
        if (s_big) {
            bg1_put((u8)(x + 1), y, BG_ENTRY(tile, PAL_NORMAL));
            bg1_put(x, (u8)(y + 1), BG_ENTRY(tile, PAL_NORMAL));
            bg1_put((u8)(x + 1), (u8)(y + 1), BG_ENTRY(tile, PAL_NORMAL));
        }
        return;
    }
    if (g_view_ink_hook && g_view_ink_hook(g_row_start[r] + c)) {
        /* Ink hides what's under it: a muted block, whatever the cell. */
        state = CELL_FILLED;
        pal = hl ? PAL_DONE_HL : PAL_DONE;
    } else if (cell == CELL_MISTAKE) {
        state = CELL_MARKED;
        pal = hl ? PAL_MISTAKE_HL : PAL_MISTAKE;
    } else {
        pal = hl ? PAL_HL : PAL_NORMAL;
    }
    if (s_big) {
        u8 variant = (u8)(s_guide[c] + (bottom ? 4 : (s_guide[r] << 1)));

        put_big(x, y, s_big_cells + (((u16)state * 6 + variant) << 2),
            (u16)pal << 10);
        return;
    }
    if (bottom)
        tile = T_CELL_B + (state << 1) + s_guide[c];
    else
        tile = T_CELL + (state << 2) + s_guide[c] + (s_guide[r] << 1);
    bg1_put(x, y, BG_ENTRY(tile, pal));
}

static u8 row_done(u8 r)
{
    return g_view_done_hook ? g_view_done_hook(r) : board_row_done(r);
}

static u8 col_done(u8 c)
{
    return g_view_done_hook ? g_view_done_hook((u8)(20 + c)) : board_col_done(c);
}

static u8 clue_pal(u8 done, u8 hl)
{
    if (done)
        return hl ? PAL_DONE_HL : PAL_DONE;
    return hl ? PAL_HL : PAL_NORMAL;
}

/* One clue (n, or a blank when n is 0xFF, or 0xFE for fog) at a tile position. */
static void put_clue(u8 x, u8 y, u8 n, u8 striped, u8 pal)
{
    u16 attr = (u16)pal << 10;

    if (s_big) {
        if (n >= 0xFE) {
            u16 blank = (striped ? T_CLUE_BLANK_ALT : T_CLUE_BLANK) | attr;

            bg1_put(x, y, blank);
            bg1_put((u8)(x + 1), y, blank);
            bg1_put(x, (u8)(y + 1), blank);
            bg1_put((u8)(x + 1), (u8)(y + 1), blank);
        } else {
            put_big(x, y, s_big_clues +
                (((u16)(striped ? BIG_MAX_CLUE + 1 : 0) + n) << 2), attr);
        }
        return;
    }
    if (n >= 0xFE)
        bg1_put(x, y, (striped ? T_CLUE_BLANK_ALT : T_CLUE_BLANK) | attr);
    else
        bg1_put(x, y, ((striped ? T_CLUE_ALT : T_CLUE) + n) | attr);
}

static void draw_row_clues(u8 r)
{
    u8 count = g_row_clue_count[r];
    u8 pal = clue_pal(s_row_done[r], r == g_play.cur_y);
    u8 y = cell_y(r);
    u8 step = (u8)(1 << s_big);
    u8 x = (u8)(g_view_bx - (g_max_row_clues << s_big));
    const u8 *clues = g_row_clues + ((u16)r << 4);

    if (g_view_clue_hook) {
        count = g_view_clue_hook(r, s_shown);
        if (count > g_max_row_clues)
            count = g_max_row_clues;
        clues = s_shown;
    }
    u8 striped = r & 1;
    u8 k;

    for (k = 0; k < g_max_row_clues - count; k++) {
        put_clue(x, y, 0xFF, striped, pal);
        x += step;
    }
    for (k = 0; k < count; k++) {
        put_clue(x, y, clues[k], striped, pal);
        x += step;
    }
}

static void draw_col_clues(u8 c)
{
    u8 count = g_col_clue_count[c];
    u8 pal = clue_pal(s_col_done[c], c == g_play.cur_x);
    u8 x = cell_x(c);
    u8 step = (u8)(1 << s_big);
    u8 y = (u8)(g_view_by - (g_max_col_clues << s_big));
    const u8 *clues = g_col_clues + ((u16)c << 4);

    if (g_view_clue_hook) {
        count = g_view_clue_hook((u8)(20 + c), s_shown);
        if (count > g_max_col_clues)
            count = g_max_col_clues;
        clues = s_shown;
    }
    u8 striped = c & 1;
    u8 k;

    for (k = 0; k < g_max_col_clues - count; k++) {
        put_clue(x, y, 0xFF, striped, pal);
        y += step;
    }
    for (k = 0; k < count; k++) {
        put_clue(x, y, clues[k], striped, pal);
        y += step;
    }
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
    u8 x = (u8)(g_view_bx - (g_max_row_clues << s_big));
    u8 n = (u8)((g_max_row_clues + g_puzzle.width) << s_big);
    u8 y = cell_y(r);

    bg1_highlight(x, y, n, 0, on);
    if (s_big)
        bg1_highlight(x, (u8)(y + 1), n, 0, on);
}

static void highlight_col(u8 c, u8 on)
{
    u8 y = (u8)(g_view_by - (g_max_col_clues << s_big));
    u8 n = (u8)((g_max_col_clues + g_puzzle.height) << s_big);
    u8 x = cell_x(c);

    bg1_highlight(x, y, n, 1, on);
    if (s_big)
        bg1_highlight((u8)(x + 1), y, n, 1, on);
}

static void layout(void)
{
    s_big = g_puzzle.width <= 10 ? 1 : 0;
    switch (g_puzzle.width) {
    case 5:
        g_view_bx = 14;
        g_view_by = 12;
        break;
    case 10:
        /* Column 10 is the HUD's ("TIME 00:00" from column 1). */
        g_view_bx = 11;
        g_view_by = 7;
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
    u16 x;
    u16 y;

    if (on == 0 || s_picture) {
        cursor_hide();
        return;
    }
    x = (u16)cell_x(g_play.cur_x) << 3;
    y = (u16)cell_y(g_play.cur_y) << 3;
    /* The small box hangs one pixel over the cell; the big one sits on it.
     * Sprites draw a line below their Y. */
    if (s_big)
        cursor_show_big(x, y - 1);
    else
        cursor_show(x - 1, y - 2);
}

/* The thumbnail sits under the HUD, where the hint menu also opens. */
void view_place_thumb(void)
{
    thumb_place(1, HUD_MENU_ROW);
}

void view_draw_hud(void)
{
    static const char *const sizes[4] = { "5X5", "10X10", "15X15", "20X20" };
    u8 i;
    u8 s = g_puzzle.width == 5 ? 0 : g_puzzle.width == 10 ? 1 :
        g_puzzle.width == 15 ? 2 : 3;

    if (g_view_hud_hook) {
        g_view_hud_hook();
        s_shown_seconds = g_play.seconds;
        s_shown_hearts = g_play.hearts;
        s_shown_hints = g_play.hints_used;
        return;
    }

    bg3_text(1, HUD_SIZE_ROW, sizes[s], TEXT_DIM);
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
    u8 y;
    u8 bottom;

    layout();
    s_picture = 0;
    bg1_clear();
    bg3_clear();
    thumb_clear(g_puzzle.width, g_puzzle.height);
    for (r = 0; r < g_puzzle.height; r++)
        s_row_done[r] = row_done(r);
    for (c = 0; c < g_puzzle.width; c++)
        s_col_done[c] = col_done(c);
    for (r = 0; r < g_puzzle.height; r++)
        draw_row(r);
    for (c = 0; c < g_puzzle.width; c++)
        draw_col_clues(c);
    /* The right edge, a column of tiles past the last cell. */
    c = cell_x(g_puzzle.width);
    bottom = (u8)(cell_y(g_puzzle.height) - 1);
    for (y = g_view_by; y <= bottom; y++)
        bg1_put(c, y, BG_ENTRY(y == bottom ? T_EDGE_RB : T_EDGE_R, PAL_NORMAL));
    s_cur_x = g_play.cur_x;
    s_cur_y = g_play.cur_y;
    g_board_dirty_count = 0;
    g_board_dirty_all = 0;
    view_draw_hud();
    if (g_view_hud_hook == 0)
        view_place_thumb();
    view_cursor(1);
}

void view_draw_picture(void)
{
    u8 r;
    u8 c;
    u8 x0 = (u8)(g_view_bx - (g_max_row_clues << s_big));
    u8 y0 = (u8)(g_view_by - (g_max_col_clues << s_big));
    u8 rows = (u8)(cell_y(g_puzzle.height) - g_view_by);

    s_picture = 1;
    for (r = 0; r < g_puzzle.height; r++)
        for (c = 0; c < g_puzzle.width; c++)
            draw_cell(r, c);
    /* No clues, no edge: just the picture. */
    bg1_fill(x0, g_view_by, (u8)(g_view_bx - x0), rows, 0);
    bg1_fill(g_view_bx, y0, (u8)((g_puzzle.width << s_big) + 1), (u8)(g_view_by - y0), 0);
    bg1_fill(cell_x(g_puzzle.width), g_view_by, 1, rows, 0);
    cursor_hide();
}

static void note_new(u8 id)
{
    if (g_view_new_count < VIEW_NEW_MAX)
        g_view_new_lines[g_view_new_count++] = id;
}

static void refresh_lines(u8 r, u8 c)
{
    u8 done = row_done(r);

    if (done != s_row_done[r]) {
        s_row_done[r] = done;
        g_view_lines_done += done;
        if (done)
            note_new(r);
        draw_row_clues(r);
    }
    done = col_done(c);
    if (done != s_col_done[c]) {
        s_col_done[c] = done;
        g_view_lines_done += done;
        if (done)
            note_new((u8)(20 + c));
        draw_col_clues(c);
    }
}

/* Redraws one line's clues (a fight changed what they show). */
void view_redraw_line(u8 id)
{
    if (id >= 20)
        draw_col_clues((u8)(id - 20));
    else
        draw_row_clues(id);
}

/* After a fight changed the board under the player (a swap's wipe, a
 * reveal): every cell, and lines re-checked without reporting them. */
void view_refresh_all(void)
{
    g_board_dirty_all = 1;
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
            s_row_done[r] = row_done(r);
            draw_row(r);
        }
        for (c = 0; c < g_puzzle.width; c++) {
            s_col_done[c] = col_done(c);
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
    if (s_shown_seconds != g_play.seconds && g_view_hud_hook == 0) {
        s_shown_seconds = g_play.seconds;
        bg3_time(6, HUD_TIME_ROW, g_play.seconds, TEXT_INK);
    }
    if (s_shown_hearts != g_play.hearts || s_shown_hints != g_play.hints_used)
        view_draw_hud();
}
