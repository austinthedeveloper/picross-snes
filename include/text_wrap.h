#ifndef TEXT_WRAP_H
#define TEXT_WRAP_H

#include "types.h"

/* Greedy word wrap of a NUL-terminated string (story text, ADR-010):
 * fills starts/lens with at most max lines of at most width characters
 * and returns how many. '\n' breaks a line. */
u8 text_wrap(const u8 *t, u8 width, u8 max, u8 *starts, u8 *lens);

/* Wraps and draws on BG3; clears the rest of the box. Returns lines used. */
u8 text_draw_wrapped(u8 x, u8 y, const u8 *t, u8 width, u8 max, u8 pal);

#endif
