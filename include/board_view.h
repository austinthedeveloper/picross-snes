#ifndef BOARD_VIEW_H
#define BOARD_VIEW_H

#include "types.h"

/* The play screen (docs/LAYOUT.md): the board's top-left tile, chosen by
 * size so every pack puzzle's clues fit and the top-left corner is free
 * for the HUD. */
extern u8 g_view_bx;
extern u8 g_view_by;

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
/* Board cursor sprite on or off. */
void view_cursor(u8 on);

/* HUD rows in the corner. */
#define HUD_SIZE_ROW 1
#define HUD_TIME_ROW 2
#define HUD_HEARTS_ROW 3
#define HUD_HINTS_ROW 4
#define HUD_MENU_ROW 5

#endif
