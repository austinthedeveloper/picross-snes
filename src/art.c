#include "art.h"
#include "video.h"
#include "art_data.h"

static const u8 *s_cast[ART_CAST_COUNT];
static const u8 *s_enemies[ART_ENEMIES_COUNT];
static const u8 *s_gadgets[ART_GADGETS_COUNT];
static const u8 *s_icons[ART_ICONS_COUNT];

void art_init(void)
{
    ART_CAST_INIT(s_cast);
    ART_ENEMIES_INIT(s_enemies);
    ART_GADGETS_INIT(s_gadgets);
    ART_ICONS_INIT(s_icons);
}

const u8 *art_image(u8 group, u8 i)
{
    if (group == ART_CAST)
        return s_cast[i];
    if (group == ART_ENEMIES)
        return s_enemies[i];
    if (group == ART_GADGETS)
        return s_gadgets[i];
    return s_icons[i];
}

void art_palette(u8 group, u8 i, u8 pal)
{
    dmaCopyCGram((u8 *)art_image(group, i), (u16)pal << 4, 32);
}

void art_upload(u8 group, u8 i, u16 tile, u8 pal, u16 bytes)
{
    const u8 *img = art_image(group, i);

    dmaCopyCGram((u8 *)img, (u16)pal << 4, 32);
    dmaCopyVram((u8 *)(img + 32), VRAM_BG1_TILES + (tile << 4), bytes);
}
