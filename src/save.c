#include "save.h"

SaveData g_save;
/* The copy read from SRAM that isn't chosen, during load. */
static SaveData s_other;
/* Which copy (0 = A, 1 = B) holds the newest save. */
static u8 s_newest;

#ifndef HOST_TEST
/* src/asm/save_sum.asm: the same sum, about 8x faster than this C. */
u16 save_sum_src;
u16 save_sum_len;
u16 save_sum_acc;
extern void save_sum(void);

u16 save_checksum(const SaveData *data)
{
    save_sum_src = (u16)data;
    save_sum_len = SAVE_BYTES;
    save_sum();
    return save_sum_acc;
}
#else
u16 save_checksum(const SaveData *data)
{
    const u8 *p = (const u8 *)data;
    u16 sum = 0x1994;
    u16 i;

    for (i = 0; i < SAVE_BYTES; i++) {
        /* The sum field itself (bytes 4 and 5) counts as zero. */
        if (i == 4 || i == 5)
            continue;
        sum = (u16)((sum << 1) | (sum >> 15));
        sum += p[i];
    }
    return sum;
}
#endif

static u8 valid(const SaveData *data)
{
    if (data->magic != SAVE_MAGIC)
        return 0;
    if (data->version == 0 || data->version > SAVE_VERSION)
        return 0;
    return data->sum == save_checksum(data);
}

void save_blank(void)
{
    u8 *p = (u8 *)&g_save;
    u16 i;

    for (i = 0; i < SAVE_BYTES; i++)
        p[i] = 0;
    g_save.magic = SAVE_MAGIC;
    g_save.version = SAVE_VERSION;
    g_save.sound = 1;
}

/* Brings an older valid layout up to SAVE_VERSION. Version 1 is the only
 * layout so far; the next change freezes it as SaveV1 here (ADR-003). */
static void migrate(void)
{
    g_save.version = SAVE_VERSION;
}

u8 save_load(void)
{
    u8 a_ok;
    u8 b_ok;

    sram_read_bytes((u8 *)&g_save, 0, SAVE_BYTES);
    sram_read_bytes((u8 *)&s_other, SAVE_COPY_B, SAVE_BYTES);
    a_ok = valid(&g_save);
    b_ok = valid(&s_other);
    if (b_ok) {
        /* B is newer when its sequence is ahead, counting wrap-around. */
        if (a_ok == 0 || (u8)(s_other.seq - g_save.seq) < 128) {
            g_save = s_other;
            s_newest = 1;
            a_ok = 1;
        } else {
            s_newest = 0;
        }
    } else {
        s_newest = 0;
    }
    if (a_ok == 0) {
        save_blank();
        return 0;
    }
    if (g_save.version != SAVE_VERSION)
        migrate();
    return 1;
}

void save_write(void)
{
    g_save.magic = SAVE_MAGIC;
    g_save.version = SAVE_VERSION;
    g_save.seq++;
    g_save.sum = save_checksum(&g_save);
    s_newest ^= 1;
    sram_write_bytes((const u8 *)&g_save, s_newest ? SAVE_COPY_B : 0,
        SAVE_BYTES);
}
