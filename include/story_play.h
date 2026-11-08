#ifndef STORY_PLAY_H
#define STORY_PLAY_H

#include "types.h"

/* A story encounter on the play screen (src/story_play.c, ADR-010). The
 * play screen calls these when g_play_story is set. */

extern u8 g_play_story;

enum { STORY_PLAYING, STORY_BUSY, STORY_KNOCKED_OUT };

/* Loads the run's current encounter: puzzle, saved board and battle, the
 * mode, and the board view's hooks. Before view_draw_all. */
void story_play_enter(void);
/* After view_draw_all. */
void story_play_drawn(void);
/* Each frame after view_update; stroke_ended when a stroke just ended.
 * STORY_BUSY: a phase card is up (no play this frame). */
u8 story_play_frame(u8 stroke_ended);
/* Y: calls out the row (col 0) or column (1) under the cursor. Returns 0
 * when this fight has nothing to call out. */
u8 story_play_call_out(u8 col);
/* Uses the i-th held gadget. Returns 0 when it can't help now. */
u8 story_play_gadget(u8 i);
void story_play_solved(void);
void story_play_draw_result(void);
/* Saves the board and battle (quitting to the map). */
void story_play_quit(void);
u8 story_play_next_scene(void);
/* Drops the hooks (leaving the play screen). */
void story_play_leave(void);

#endif
