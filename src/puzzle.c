#include "puzzle.h"

/* pack_info: 8 bytes a slot (docs/PUZZLES.md):
 *   u16 bits offset, u16 name offset, width, height, passes, collection. */
ROM_DECLARE(pack_info);
ROM_DECLARE(pack_bits);
ROM_DECLARE(pack_names);
/* pack_cinfo: 8 bytes a collection: u16 first, count, medal, u16 name,
 * u16 emblem slot. pack_cslots: u16 slots in display order. */
ROM_DECLARE(pack_cinfo);
ROM_DECLARE(pack_cslots);
ROM_DECLARE(pack_cnames);

PuzzleHeader g_puzzle;
u8 g_solution[MAX_CELLS];
u16 g_row_start[MAX_SIDE];
u8 g_row_clues[MAX_SIDE * CLUE_STRIDE];
u8 g_col_clues[MAX_SIDE * CLUE_STRIDE];
u8 g_row_clue_count[MAX_SIDE];
u8 g_col_clue_count[MAX_SIDE];
u8 g_max_row_clues;
u8 g_max_col_clues;

static const u8 *info(u16 slot)
{
    return ROM_DATA(pack_info) + (slot << 3);
}

static u16 read16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

const u8 *pack_info_table(void)
{
    return ROM_DATA(pack_info);
}

u8 pack_width(u16 slot)
{
    return info(slot)[4];
}

u8 pack_height(u16 slot)
{
    return info(slot)[5];
}

u8 pack_passes(u16 slot)
{
    return info(slot)[6];
}

u8 pack_collection(u16 slot)
{
    return info(slot)[7];
}

const u8 *pack_name(u16 slot)
{
    return ROM_DATA(pack_names) + read16(info(slot) + 2);
}

u8 pack_points(u16 slot)
{
    u8 side = pack_width(slot);

    if (side <= 5)
        return 1;
    if (side <= 10)
        return 2;
    if (side <= 15)
        return 3;
    return 6;
}

u8 pack_band(u16 slot)
{
    /* Web cutoffs (src/puzzles/difficulty.ts): 0, 3, 5, 8, 11 passes. */
    u8 p = pack_passes(slot);

    if (p >= 11)
        return 4;
    if (p >= 8)
        return 3;
    if (p >= 5)
        return 2;
    if (p >= 3)
        return 1;
    return 0;
}

static const u8 *cinfo(u8 c)
{
    return ROM_DATA(pack_cinfo) + ((u16)c << 3);
}

u8 collection_size(u8 c)
{
    return cinfo(c)[2];
}

u16 collection_slot(u8 c, u8 i)
{
    u16 first = read16(cinfo(c));

    return read16(ROM_DATA(pack_cslots) + ((first + i) << 1));
}

const u8 *collection_name(u8 c)
{
    return ROM_DATA(pack_cnames) + read16(cinfo(c) + 4);
}

u16 collection_emblem(u8 c)
{
    return read16(cinfo(c) + 6);
}

/* Appends one line's runs as clues; returns the count. */
static u8 line_clues(u8 *out, const u8 *cells, u16 step, u8 length)
{
    u8 count = 0;
    u8 run = 0;
    u8 i;

    for (i = 0; i < length; i++) {
        if (*cells) {
            run++;
        } else if (run) {
            out[count++] = run;
            run = 0;
        }
        cells += step;
    }
    if (run)
        out[count++] = run;
    if (count == 0)
        out[count++] = 0;
    return count;
}

void puzzle_load(u16 slot)
{
    const u8 *p = info(slot);
    const u8 *bits = ROM_DATA(pack_bits) + read16(p);
    u8 w = p[4];
    u8 h = p[5];
    u8 row_bytes = (u8)((w + 7) >> 3);
    u8 r;
    u8 c;
    u16 index = 0;
    u8 count;

    g_puzzle.slot = slot;
    g_puzzle.width = w;
    g_puzzle.height = h;
    g_puzzle.filled = 0;
    for (r = 0; r < h; r++) {
        u8 mask = 0x80;
        const u8 *byte = bits;

        g_row_start[r] = index;
        for (c = 0; c < w; c++) {
            u8 on = (*byte & mask) ? 1 : 0;

            g_solution[index++] = on;
            g_puzzle.filled += on;
            mask >>= 1;
            if (mask == 0) {
                mask = 0x80;
                byte++;
            }
        }
        bits += row_bytes;
    }
    g_puzzle.cells = index;

    g_max_row_clues = 0;
    for (r = 0; r < h; r++) {
        count = line_clues(g_row_clues + ((u16)r << 4),
            g_solution + g_row_start[r], 1, w);
        g_row_clue_count[r] = count;
        if (count > g_max_row_clues)
            g_max_row_clues = count;
    }
    g_max_col_clues = 0;
    for (c = 0; c < w; c++) {
        count = line_clues(g_col_clues + ((u16)c << 4), g_solution + c, w, h);
        g_col_clue_count[c] = count;
        if (count > g_max_col_clues)
            g_max_col_clues = count;
    }
}
