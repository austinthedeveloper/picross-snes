#ifndef BOARD_VIEW_H
#define BOARD_VIEW_H

#include "types.h"

/* The play screen (docs/LAYOUT.md): the board's top-left tile, chosen by
 * size so every pack puzzle's clues fit and the top-left corner is free
 * for the HUD. */
extern u8 g_view_bx;
extern u8 g_view_by;

/* Lines that turned finished in view_update (for the line chime). */
extern u8 g_view_lines_done;

/* Story fights (ADR-010). Each NULL in free play:
 * - clue hook: the clue a line shows (line ids as battle.h); 0xFE blanks a
 *   number (fog);
 * - done hook: whether a line is complete as shown (the web's rule);
 * - ink hook: a cell is under ink;
 * - HUD hook: draws the corner instead of time, hearts and hints.
 * view_update collects lines that turned complete (not the first draw)
 * in g_view_new_lines for the fight to read and reset. */
#define VIEW_NEW_MAX 16
extern u8 (*g_view_clue_hook)(u8 id, u8 *out);
extern u8 (*g_view_done_hook)(u8 id);
extern u8 (*g_view_ink_hook)(u16 index);
extern void (*g_view_hud_hook)(void);
extern u8 g_view_new_lines[VIEW_NEW_MAX];
extern u8 g_view_new_count;
void view_redraw_line(u8 id);
void view_refresh_all(void);

/* Draws the whole board, its clues and the HUD into the shadow maps. */
void view_draw_all(void);
/* After play changes: redraws dirty cells, finished clues, a moved cursor,
 * and the HUD numbers. Call once a frame. */
void view_update(void);
/* The solved picture: no grid, no clues. */
void view_draw_picture(void);
/* The HUD rows (time, hearts, hints). */
void view_draw_hud(void);
/* Clears HUD rows from..to-1 (messages and menus below the HUD). */
void view_clear_corner(u8 from, u8 to);
/* The thumbnail's tiles back on the map (after a menu covered them). */
void view_place_thumb(void);
/* Board cursor sprite on or off. */
void view_cursor(u8 on);

/* HUD rows in the corner. */
#define HUD_SIZE_ROW 1
#define HUD_TIME_ROW 2
#define HUD_HEARTS_ROW 3
#define HUD_HINTS_ROW 4
#define HUD_MENU_ROW 5

#endif
