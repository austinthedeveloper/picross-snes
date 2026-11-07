#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"
#include "sfx.h"

/* Sound effects through SNESMOD (ADR-006). The sound CPU takes queued
 * commands in audio_update, once a frame while any are pending. */
void audio_init(void);
/* volume 0-15, pan 0 (left) - 15 (right), 8 the middle. */
void audio_play(u8 sfx, u8 volume);
void audio_update(void);

#endif
