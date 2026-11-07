#include "audio.h"
#include "save.h"

/* The soundbank (smconv, bank 6): one bank holds it all. */
extern char SOUNDBANK__;

/* Frames left to keep handing queued commands to the sound CPU: one
 * spcProcess call can leave some queued (Pickleball '94 ADR-031). */
static u8 s_pending;
#define AUDIO_PROCESS_FRAMES 6

void audio_init(void)
{
    u8 i;

    s_pending = 0;
    spcBoot();
    spcSetBank((u8 *)&SOUNDBANK__);
    spcStop();
    for (i = 0; i < SFX_COUNT; i++)
        spcLoadEffect(i);
    s_pending = AUDIO_PROCESS_FRAMES;
}

void audio_play(u8 sfx, u8 volume)
{
    if (g_save.sound == 0)
        return;
    spcEffect(SFX_PITCH, sfx, (u8)((volume << 4) | 8));
    s_pending = AUDIO_PROCESS_FRAMES;
}

void audio_update(void)
{
    if (s_pending == 0)
        return;
    s_pending--;
    spcProcess();
}
