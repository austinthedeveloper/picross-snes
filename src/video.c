#include "video.h"
#include "tiles.h"

ROM_DECLARE(font_pic);
ROM_DECLARE(board_pic);
ROM_DECLARE(obj_pic);
ROM_DECLARE(palette_pal);
extern char font_pic_bin_end, board_pic_bin_end, obj_pic_bin_end;

u16 g_bg1_map[1024];
u16 g_bg3_map[1024];
u8 g_bg1_dirty;
u8 g_bg3_dirty;

extern void map_commit(void);

/* src/asm/map_draw.asm's inputs and output. */
const char *md_src;
u16 md_dst;
u16 md_attr;
u16 md_max;
u16 md_pad;
u16 md_count;
u16 md_w;
u16 md_h;
u16 md_entry;
extern void map_text(void);
extern void map_fill(void);
extern void map_hl(void);
u16 wm_src;
u16 wm_dst;
u16 wm_len;
extern void wram_move(void);
static void map_fill_rect(u16 *map, u8 x, u8 y, u8 w, u8 h, u16 entry);

void video_init(void)
{
    setScreenOff();
    dmaCopyCGram((u8 *)ROM_DATA(palette_pal), 0, 512);
    dmaCopyVram((u8 *)ROM_DATA(board_pic), VRAM_BG1_TILES,
        (u16)(&board_pic_bin_end - (char *)ROM_DATA(board_pic)));
    dmaCopyVram((u8 *)ROM_DATA(font_pic), VRAM_BG3_TILES,
        (u16)(&font_pic_bin_end - (char *)ROM_DATA(font_pic)));
    dmaCopyVram((u8 *)ROM_DATA(obj_pic), VRAM_OBJ_TILES,
        (u16)(&obj_pic_bin_end - (char *)ROM_DATA(obj_pic)));
    bgSetGfxPtr(0, VRAM_BG1_TILES);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetGfxPtr(2, VRAM_BG3_TILES);
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    bgSetDisable(1);
    bgSetEnable(0);
    bgSetEnable(2);
    oamInitGfxAttr(VRAM_OBJ_TILES, OBJ_SIZE8_L16);
    oamClear(0, 0);
    bgSetScroll(0, 0, 0);
    bgSetScroll(2, 0, 0);
    bg1_clear();
    bg3_clear();
    dmaCopyVram((u8 *)g_bg1_map, VRAM_BG1_MAP, 2048);
    dmaCopyVram((u8 *)g_bg3_map, VRAM_BG3_MAP, 2048);
    g_bg1_dirty = 0;
    g_bg3_dirty = 0;
}

void video_commit(void)
{
    map_commit();
}

void video_frame(void)
{
    WaitForVBlank();
    video_commit();
}

void video_fade_out(void)
{
    s8 level;

    for (level = 14; level >= 0; level -= 2) {
        WaitForVBlank();
        video_commit();
        setBrightness((u8)level);
    }
    WaitForVBlank();
    setScreenOff();
}

void video_fade_in(void)
{
    u8 level;

    WaitForVBlank();
    dmaCopyVram((u8 *)g_bg1_map, VRAM_BG1_MAP, 2048);
    dmaCopyVram((u8 *)g_bg3_map, VRAM_BG3_MAP, 2048);
    g_bg1_dirty = 0;
    g_bg3_dirty = 0;
    setScreenOn();
    for (level = 1; level <= 15; level += 2) {
        setBrightness(level);
        WaitForVBlank();
        video_commit();
    }
    setBrightness(15);
}

void bg1_clear(void)
{
    map_fill_rect(g_bg1_map, 0, 0, 32, 32, 0);
    g_bg1_dirty = 0x0F;
}

void bg3_clear(void)
{
    map_fill_rect(g_bg3_map, 0, 0, 32, 32, 0);
    g_bg3_dirty = 0x0F;
}

void bg1_put(u8 x, u8 y, u16 entry)
{
    g_bg1_map[((u16)y << 5) | x] = entry;
    g_bg1_dirty |= (u8)(1 << (y >> 3));
}

/* Bits for the 8-row bands rows y..y+h-1 touch. */
static u8 band_bits(u8 y, u8 h)
{
    u8 bits = 0;
    u8 b = (u8)(y >> 3);
    u8 last = (u8)((y + h - 1) >> 3);

    while (b <= last) {
        bits |= (u8)(1 << b);
        b++;
    }
    return bits;
}

static void map_fill_rect(u16 *map, u8 x, u8 y, u8 w, u8 h, u16 entry)
{
    md_dst = (u16)(map + ((u16)y << 5) + x);
    md_w = w;
    md_h = h;
    md_entry = entry;
    map_fill();
}

void maps_scroll_rows(u8 y, u8 rows, u8 up)
{
    /* Rows y..y+rows-1 of both maps move one row up (or down). */
    u16 top = (u16)y << 5;
    u16 bytes = (u16)(rows - 1) << 6;

    wm_len = bytes;
    if (up) {
        wm_src = (u16)(g_bg1_map + top + 32);
        wm_dst = (u16)(g_bg1_map + top);
        wram_move();
        wm_src = (u16)(g_bg3_map + top + 32);
        wm_dst = (u16)(g_bg3_map + top);
        wram_move();
    } else {
        wm_src = (u16)(g_bg1_map + top);
        wm_dst = (u16)(g_bg1_map + top + 32);
        wram_move();
        wm_src = (u16)(g_bg3_map + top);
        wm_dst = (u16)(g_bg3_map + top + 32);
        wram_move();
    }
    g_bg1_dirty |= band_bits(y, rows);
    g_bg3_dirty |= band_bits(y, rows);
}

void bg1_highlight(u8 x, u8 y, u8 n, u8 down, u8 on)
{
    md_dst = (u16)(g_bg1_map + ((u16)y << 5) + x);
    md_w = n;
    md_h = down ? 64 : 2;
    md_entry = on;
    map_hl();
    g_bg1_dirty |= down ? band_bits(y, n) : (u8)(1 << (y >> 3));
}

void bg1_fill(u8 x, u8 y, u8 w, u8 h, u16 entry)
{
    if (w == 0 || h == 0)
        return;
    map_fill_rect(g_bg1_map, x, y, w, h, entry);
    g_bg1_dirty |= band_bits(y, h);
}

void bg1_frame(u8 x, u8 y, u8 w, u8 h)
{
    u8 i;
    u8 x2 = (u8)(x + w - 1);
    u8 y2 = (u8)(y + h - 1);

    bg1_fill((u8)(x + 1), (u8)(y + 1), (u8)(w - 2), (u8)(h - 2),
        BG_ENTRY(T_FRAME + 4, 2));
    for (i = (u8)(x + 1); i < x2; i++) {
        bg1_put(i, y, BG_ENTRY(T_FRAME + 1, 2));
        bg1_put(i, y2, BG_ENTRY(T_FRAME + 7, 2));
    }
    for (i = (u8)(y + 1); i < y2; i++) {
        bg1_put(x, i, BG_ENTRY(T_FRAME + 3, 2));
        bg1_put(x2, i, BG_ENTRY(T_FRAME + 5, 2));
    }
    bg1_put(x, y, BG_ENTRY(T_FRAME, 2));
    bg1_put(x2, y, BG_ENTRY(T_FRAME + 2, 2));
    bg1_put(x, y2, BG_ENTRY(T_FRAME + 6, 2));
    bg1_put(x2, y2, BG_ENTRY(T_FRAME + 8, 2));
}

void bg3_put(u8 x, u8 y, u16 entry)
{
    g_bg3_map[((u16)y << 5) | x] = entry;
    g_bg3_dirty |= (u8)(1 << (y >> 3));
}

void bg3_fill(u8 x, u8 y, u8 w, u8 h, u16 entry)
{
    if (w == 0 || h == 0)
        return;
    map_fill_rect(g_bg3_map, x, y, w, h, entry);
    g_bg3_dirty |= band_bits(y, h);
}

u8 bg3_text(u8 x, u8 y, const char *s, u8 pal)
{
    if (x >= 32)
        return x;
    md_src = s;
    md_dst = (u16)(g_bg3_map + (((u16)y << 5) | x));
    md_attr = ((u16)pal << 10) | BG_PRIO;
    md_max = (u16)(32 - x);
    md_pad = 0;
    map_text();
    g_bg3_dirty |= (u8)(1 << (y >> 3));
    return (u8)(x + md_count);
}

void bg3_text_n(u8 x, u8 y, const char *s, u8 n, u8 pal)
{
    md_src = s;
    md_dst = (u16)(g_bg3_map + (((u16)y << 5) | x));
    md_attr = ((u16)pal << 10) | BG_PRIO;
    md_max = n;
    md_pad = 1;
    map_text();
    g_bg3_dirty |= (u8)(1 << (y >> 3));
}

static const u16 s_pow10[5] = { 10000, 1000, 100, 10, 1 };

void bg3_number(u8 x, u8 y, u16 n, u8 width, u8 zeros, u8 pal)
{
    /* Digits by subtraction: 816-tcc's divide is a slow library call. */
    char buf[6];
    u8 digits[5];
    u8 i;
    u8 d;
    u8 k = 0;
    u8 lead = zeros ? 0 : 1;

    for (i = 0; i < 5; i++) {
        d = 0;
        while (n >= s_pow10[i]) {
            n -= s_pow10[i];
            d++;
        }
        digits[i] = d;
    }
    for (i = (u8)(5 - width); i < 5; i++) {
        if (lead && digits[i] == 0 && i < 4) {
            buf[k++] = ' ';
        } else {
            lead = 0;
            buf[k++] = (char)('0' + digits[i]);
        }
    }
    buf[k] = 0;
    bg3_text(x, y, buf, pal);
}

void bg3_time(u8 x, u8 y, u16 seconds, u8 pal)
{
    u8 minutes = 0;
    char buf[6];

    if (seconds == 0xFFFF) {
        bg3_text(x, y, "--:--", pal);
        return;
    }
    while (seconds >= 600 && minutes < 100) {
        seconds -= 600;
        minutes += 10;
    }
    while (seconds >= 60) {
        seconds -= 60;
        minutes++;
    }
    if (minutes > 99) {
        bg3_text(x, y, "99:59", pal);
        return;
    }
    buf[0] = '0';
    while (minutes >= 10) {
        minutes -= 10;
        buf[0]++;
    }
    buf[1] = (char)('0' + minutes);
    buf[2] = ':';
    buf[3] = '0';
    while (seconds >= 10) {
        seconds -= 10;
        buf[3]++;
    }
    buf[4] = (char)('0' + seconds);
    buf[5] = 0;
    bg3_text(x, y, buf, pal);
}

void bg3_center(u8 y, const char *s, u8 pal)
{
    u8 n = 0;

    while (s[n])
        n++;
    bg3_text((u8)((32 - n) >> 1), y, s, pal);
}

void cursor_show(u16 x, u16 y)
{
    oamSet(0, x, y, 3, 0, 0, 0, 0);
    oamSetEx(0, OBJ_LARGE, OBJ_SHOW);
}

void cursor_hide(void)
{
    oamSetEx(0, OBJ_LARGE, OBJ_HIDE);
}
