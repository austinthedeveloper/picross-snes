/* SRAM for the host tests: an array, with a hook to tear writes. */
#include <string.h>
#include "save.h"

u8 g_host_sram[SAVE_SRAM_BYTES];
/* When nonzero, the next write stops after this many bytes (a torn write). */
u16 g_host_tear_after;

void sram_write_bytes(const u8 *src, u16 offset, u16 len)
{
    if (g_host_tear_after && g_host_tear_after < len) {
        len = g_host_tear_after;
        g_host_tear_after = 0;
    }
    memcpy(g_host_sram + offset, src, len);
}

void sram_read_bytes(u8 *dest, u16 offset, u16 len)
{
    memcpy(dest, g_host_sram + offset, len);
}
