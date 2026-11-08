#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

/* VRAM layout in words (docs/LAYOUT.md). */
#define VRAM_BG1_TILES 0x0000
#define VRAM_BG3_TILES 0x4000
#define VRAM_BG1_MAP 0x5000
#define VRAM_BG3_MAP 0x5400
#define VRAM_OBJ_TILES 0x6000

/* A tilemap entry: tile, palette, priority. */
#define BG_ENTRY(tile, pal) ((u16)(tile) | ((u16)(pal) << 10))
#define BG_PRIO 0x2000

/* Text palettes (BG3, tools/gen_assets.py). */
enum {
    TEXT_INK = 0, TEXT_DIM = 1, TEXT_SELECTED = 2, TEXT_LIGHT = 3,
    TEXT_OK = 4, TEXT_BRONZE = 5, TEXT_SILVER = 6, TEXT_GOLD = 7
};

/* Shadow tilemaps, uploaded in 8-row bands by map_commit (asm). */
extern u16 g_bg1_map[1024];
extern u16 g_bg3_map[1024];
extern u8 g_bg1_dirty;
extern u8 g_bg3_dirty;

void video_init(void);
/* Right after WaitForVBlank: the dirty map bands. */
void video_commit(void);
/* Waits for the next frame and commits (the main loop's frame step). */
void video_frame(void);
/* Fades to black and holds forced blank (scene changes redraw then). */
void video_fade_out(void);
/* Uploads both shadow maps whole and fades in. */
void video_fade_in(void);

void bg1_clear(void);
void bg3_clear(void);
void bg1_put(u8 x, u8 y, u16 entry);
void bg1_fill(u8 x, u8 y, u8 w, u8 h, u16 entry);
/* Scrolls rows y..y+rows-1 of both maps by one row, up or down; the row
 * that comes in keeps its old contents for the caller to redraw. */
void maps_scroll_rows(u8 y, u8 rows, u8 up);
/* Sets or clears palette bit 0 (the board highlight) on n entries going
 * right, or down when down is 1. */
void bg1_highlight(u8 x, u8 y, u8 n, u8 down, u8 on);
/* A frame (rounded card) of w x h tiles. */
void bg1_frame(u8 x, u8 y, u8 w, u8 h);
void bg3_put(u8 x, u8 y, u16 entry);
void bg3_fill(u8 x, u8 y, u8 w, u8 h, u16 entry);
/* Text; '\n' is not handled. Returns the column after the text. */
u8 bg3_text(u8 x, u8 y, const char *s, u8 pal);
/* At most n characters, padded with spaces to n. */
void bg3_text_n(u8 x, u8 y, const char *s, u8 n, u8 pal);
/* A number right-aligned in width characters (spaces before it), or
 * zero-padded when zeros is 1. */
void bg3_number(u8 x, u8 y, u16 n, u8 width, u8 zeros, u8 pal);
/* "MM:SS" (or "--:--" when seconds is 0xFFFF). */
void bg3_time(u8 x, u8 y, u16 seconds, u8 pal);
/* Centered text on the 32-column screen. */
void bg3_center(u8 y, const char *s, u8 pal);

/* The board cursor sprite. */
void cursor_show(u16 x, u16 y);
void cursor_hide(void);

#endif
