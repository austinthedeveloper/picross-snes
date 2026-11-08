#include "text_wrap.h"
#include "video.h"

u8 text_wrap(const u8 *t, u8 width, u8 max, u8 *starts, u8 *lens)
{
    u8 pos = 0;
    u8 start;
    u8 col;
    u8 brk;
    u8 lines = 0;

    while (t[pos] && lines < max) {
        while (t[pos] == ' ')
            pos++;
        if (t[pos] == 0)
            break;
        start = pos;
        col = 0;
        brk = 0xFF;
        while (t[start + col] && t[start + col] != '\n' && col < width) {
            if (t[start + col] == ' ')
                brk = col;
            col++;
        }
        if (t[start + col] && t[start + col] != ' ' && t[start + col] != '\n'
            && brk != 0xFF)
            col = brk;
        starts[lines] = start;
        lens[lines] = col;
        lines++;
        pos = start + col;
        if (t[pos] == '\n')
            pos++;
    }
    return lines;
}

u8 text_draw_wrapped(u8 x, u8 y, const u8 *t, u8 width, u8 max, u8 pal)
{
    static u8 starts[8];
    static u8 lens[8];
    u8 n;
    u8 i;
    u8 k;

    bg3_fill(x, y, width, max, 0);
    if (t == 0)
        return 0;
    if (max > 8)
        max = 8;
    n = text_wrap(t, width, max, starts, lens);
    for (i = 0; i < n; i++) {
        for (k = 0; k < lens[i]; k++)
            bg3_put(x + k, y + i, BG_ENTRY(t[starts[i] + k] - 32, pal) | BG_PRIO);
    }
    return n;
}
