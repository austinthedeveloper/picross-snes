#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/* Controller 1 this frame: held, newly pressed, and pressed-or-repeating
 * (the d-pad repeats after INPUT_REPEAT_DELAY frames, every
 * INPUT_REPEAT_RATE). Read once a frame by input_update. */
#define INPUT_REPEAT_DELAY 14
#define INPUT_REPEAT_RATE 4
#define PAD_DIRS (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)

extern u16 g_held;
extern u16 g_pressed;
extern u16 g_repeat;

void input_update(void);
/* Swallows the buttons held now: they don't count as pressed until they
 * are let go (a screen's closing press never acts on the next screen). */
void input_swallow(void);

#endif
