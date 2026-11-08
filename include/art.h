#ifndef ART_H
#define ART_H

#include "types.h"

/* Portraits, enemy sprites and icons in ROM (tools/gen_art.py): each a
 * 32-byte palette then 4bpp tiles, row by row. Indexes follow
 * tools/gen_story.py (sorted by id). SNES only: the host tests never draw. */

enum { ART_CAST, ART_ENEMIES, ART_GADGETS, ART_ICONS };

void art_init(void);
const u8 *art_image(u8 group, u8 i);

/* Uploads an image's tiles to BG1 tile `tile` on and its palette to BG
 * palette `pal`. In forced blank (or a VBlank with room: a 64x64 portrait
 * is 2 KiB). bytes: the tile data's size (64 * 32 for a portrait). */
void art_upload(u8 group, u8 i, u16 tile, u8 pal, u16 bytes);
/* Just the palette, to BG palette pal. */
void art_palette(u8 group, u8 i, u8 pal);

#endif
