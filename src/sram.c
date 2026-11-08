#include "save.h"

/* src/asm/sram_copy.asm: MVN between work RAM and SRAM. */
u16 sram_copy_wram;
u16 sram_copy_sram;
u16 sram_copy_len;
extern void sram_copy_out(void);
extern void sram_copy_in(void);

void sram_write_bytes(const u8 *src, u16 offset, u16 len)
{
    sram_copy_wram = (u16)src;
    sram_copy_sram = offset;
    sram_copy_len = len;
    sram_copy_out();
}

void sram_read_bytes(u8 *dest, u16 offset, u16 len)
{
    sram_copy_wram = (u16)dest;
    sram_copy_sram = offset;
    sram_copy_len = len;
    sram_copy_in();
}
