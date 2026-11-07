#ifndef PLAY_H
#define PLAY_H

#include "board.h"

/* How mistakes work (web: src/game/game.ts): relaxed never checks fills;
 * hearts checks each fill, marks a wrong one, and fails after three. */
enum { MODE_RELAXED = 0, MODE_HEARTS = 1 };
#define MAX_HEARTS 3
#define MAX_HINTS 3

/* A stroke paints one action over every cell the cursor crosses while its
 * button is held (web: drag strokes). The first cell decides the action. */
enum {
    STROKE_NONE = 0,
    STROKE_FILL,     /* empty -> filled */
    STROKE_ERASE,    /* filled -> empty */
    STROKE_MARK,     /* empty -> marked */
    STROKE_UNMARK,   /* marked -> empty */
    STROKE_BLOCKED   /* a mistake ended the stroke; wait for release */
};
enum { HINT_CELL = 0, HINT_ROW = 1, HINT_COL = 2 };

typedef struct {
    u8 mode;
    u8 hearts;
    u8 hints_used;
    u8 mistakes;
    u8 solved;
    u8 failed;
    u8 stroke;
    u8 cur_x;
    u8 cur_y;
    /* Frames into the current second, and whole seconds played. */
    u8 frames;
    u16 seconds;
} Play;

extern Play g_play;

/* Loads the slot's puzzle and starts it fresh (the cursor at the top
 * left, the clock at zero). */
void play_start(u16 slot, u8 mode);
/* Starts over with the same puzzle and mode. */
void play_restart(void);
/* button_a: 1 for the fill button, 0 for the mark button. */
void play_stroke_begin(u8 button_a);
/* Applies the stroke to the cell under the cursor (after a move). */
void play_stroke_continue(void);
void play_stroke_end(void);
u8 play_undo(void);
u8 play_redo(void);
/* Reveals the target under the cursor; costs a hint when it changed
 * anything. Returns the cells it changed (0: nothing, or no hints left). */
u8 play_hint(u8 target);
/* One frame of play: the clock. */
void play_tick(void);
/* True when the result can count as perfect: hearts mode, no mistakes,
 * no hints. */
u8 play_perfect(void);

#endif
