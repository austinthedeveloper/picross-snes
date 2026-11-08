#include "art.h"
#include "dialogue.h"
#include "input.h"
#include "scene.h"
#include "story.h"
#include "tiles.h"
#include "video.h"

/* The scene player (ADR-010; the web's ui/components/dialogue.ts): the
 * backdrop on BG2, the cast's 64x64 portraits on BG1 above a text card,
 * the speaker's name and typed text on BG3, choices with a pointer. A or B
 * finishes the line or moves on; START skips the scene, as the web's Skip.
 *
 * VRAM: portraits sit in BG1 tiles PORTRAIT_TILE on (64 a cast member,
 * at most 4 in a scene), past the board's tiles, so the board needs no
 * reload; two tiles at the very end are the backdrop's solid and blank
 * tiles. Palettes: BG 5-7 are the three places on stage, BG 4 the
 * backdrop; video_restore_palette() puts the board's back on leaving. */

u8 g_story_scene;
u8 g_story_flags;
u8 g_story_next;

#define PORTRAIT_TILE 512
#define PORTRAIT_TILES 64
#define SOLID_TILE 1023
#define BLANK_TILE 1022
#define PAL_BACKDROP 4
#define PAL_STAGE 5
#define STAGE_Y 10
#define BOX_Y 18
#define BOX_H 10
#define TEXT_X 2
#define TEXT_Y 20
#define TEXT_W 28
#define TEXT_LINES 6
#define TYPE_SPEED 2

static const u8 s_stage_x[STAGE_SIZE] = { 2, 12, 22 };

static Dialogue s_dlg;
static Step s_step;
static Stage s_stage;
static u8 s_slot_who[STAGE_SIZE];
static const u8 *s_scene;
static const u8 *s_text;
static u8 s_line_start[TEXT_LINES];
static u8 s_line_len[TEXT_LINES];
static u8 s_lines;
static u8 s_typed_line;
static u8 s_typed_col;
static u8 s_typing;
static u8 s_choice;
static u16 s_backdrop_map[32 * 28];
static u8 s_tile_buf[32];

/* Greedy word wrap into at most TEXT_LINES lines of TEXT_W. */
static void wrap(const u8 *t)
{
    u8 pos = 0;
    u8 start;
    u8 col;
    u8 brk;

    s_lines = 0;
    while (t[pos] && s_lines < TEXT_LINES) {
        while (t[pos] == ' ')
            pos++;
        if (t[pos] == 0)
            break;
        start = pos;
        col = 0;
        brk = 0xFF;
        while (t[start + col] && t[start + col] != '\n' && col < TEXT_W) {
            if (t[start + col] == ' ')
                brk = col;
            col++;
        }
        if (t[start + col] && t[start + col] != ' ' && t[start + col] != '\n'
            && brk != 0xFF)
            col = brk;
        s_line_start[s_lines] = start;
        s_line_len[s_lines] = col;
        s_lines++;
        pos = start + col;
        if (t[pos] == '\n')
            pos++;
    }
}

static void clear_box(void)
{
    bg3_fill(TEXT_X, BOX_Y + 1, TEXT_W, BOX_H - 2, 0);
}

static void draw_portrait(u8 slot, u8 who)
{
    u8 x;
    u8 y;
    u16 tile = PORTRAIT_TILE + who * PORTRAIT_TILES;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            bg1_put(s_stage_x[slot] + x, STAGE_Y + y,
                BG_ENTRY(tile, PAL_STAGE + slot));
            tile++;
        }
    }
}

/* Puts the cast member on stage (web speakOnStage); a newcomer brings
 * their palette, written in the next VBlank. */
static void speak(u8 who)
{
    u8 slot = stage_speak(&s_stage, who);

    if (s_slot_who[slot] == who)
        return;
    s_slot_who[slot] = who;
    video_queue_palette(art_image(ART_CAST, scene_cast_portrait(s_scene, who)),
        PAL_STAGE + slot);
    draw_portrait(slot, who);
}

static void show_step(void)
{
    const u8 *name;

    dialogue_step(&s_dlg, &s_step);
    clear_box();
    if (s_step.kind == STEP_END)
        return;
    if (s_step.kind == STEP_LINE) {
        if (s_step.who != STORY_NONE) {
            speak(s_step.who);
            name = story_string(scene_cast_name(s_scene, s_step.who));
            bg3_text(TEXT_X, BOX_Y + 1, (const char *)name, TEXT_SELECTED);
        }
        s_text = story_string(s_step.text);
        wrap(s_text);
        s_typed_line = 0;
        s_typed_col = 0;
        s_typing = 1;
        return;
    }
    s_choice = 0;
    s_typing = 0;
    {
        u8 i;
        const u8 *node = scene_node(s_scene, s_dlg.node);
        for (i = 0; i < s_step.choices; i++) {
            const u8 *c = node_choice(node, s_step.choice[i]);
            bg3_text_n(TEXT_X + 2, TEXT_Y + i, (const char *)story_string(CHOICE_TEXT(c)),
                TEXT_W - 2, i == 0 ? TEXT_SELECTED : TEXT_INK);
        }
        menu_pointer(TEXT_X, TEXT_Y, 1);
    }
}

/* Types the next few characters of the line. */
static void type_more(u8 n)
{
    u8 ch;
    u8 pal = s_step.who == STORY_NONE ? TEXT_DIM : TEXT_INK;

    while (n && s_typing) {
        if (s_typed_line >= s_lines) {
            s_typing = 0;
            bg3_put(TEXT_X + TEXT_W - 1, BOX_Y + BOX_H - 2,
                BG_ENTRY('^' - 32, TEXT_SELECTED) | BG_PRIO);
            break;
        }
        if (s_typed_col >= s_line_len[s_typed_line]) {
            s_typed_line++;
            s_typed_col = 0;
            continue;
        }
        ch = s_text[s_line_start[s_typed_line] + s_typed_col];
        bg3_put(TEXT_X + s_typed_col, TEXT_Y + s_typed_line,
            BG_ENTRY(ch - 32, pal) | BG_PRIO);
        s_typed_col++;
        n--;
    }
}

static void draw_backdrop(u8 b)
{
    const u8 *bd;
    u8 w;
    u8 h;
    u8 bytes;
    u8 x;
    u8 y;
    s16 row;
    const u8 *bits;
    u16 entry;

    for (x = 0; x < 32; x++)
        for (y = 0; y < 28; y++)
            s_backdrop_map[((u16)y << 5) + x] = BLANK_TILE;
    if (b != STORY_NONE) {
        bd = story_backdrop(b);
        w = bd[0];
        h = bd[1];
        bytes = (u8)((w + 7) >> 3);
        /* Bottom-anchored, cropped to the screen (the web's xMidYMax slice). */
        for (y = 0; y < 28; y++) {
            row = (s16)y + (s16)h - 28;
            if (row < 0)
                continue;
            bits = bd + 2 + (u16)row * bytes;
            for (x = 0; x < 32 && x < w; x++) {
                entry = (bits[x >> 3] & (0x80 >> (x & 7)))
                    ? BG_ENTRY(SOLID_TILE, PAL_BACKDROP) : BLANK_TILE;
                s_backdrop_map[((u16)y << 5) + x] = entry;
            }
        }
    }
    dmaCopyVram((u8 *)s_backdrop_map, VRAM_BG2_MAP, sizeof s_backdrop_map);
}

void dialogue_enter(void)
{
    u8 i;
    static u8 backdrop_pal[32];
    u16 c = COLOUR_BACKDROP;

    s_scene = story_scene(g_story_scene);
    bg1_clear();
    bg3_clear();
    /* The backdrop's two tiles and colour. */
    for (i = 0; i < 32; i++)
        s_tile_buf[i] = 0;
    dmaCopyVram(s_tile_buf, VRAM_BG1_TILES + (BLANK_TILE << 4), 32);
    for (i = 0; i < 16; i++)
        s_tile_buf[i] = (i & 1) ? 0x00 : 0xFF;  /* plane 0 on every row: colour 1 */
    dmaCopyVram(s_tile_buf, VRAM_BG1_TILES + (SOLID_TILE << 4), 32);
    for (i = 0; i < 32; i++)
        backdrop_pal[i] = 0;
    backdrop_pal[2] = (u8)(c & 0xFF);
    backdrop_pal[3] = (u8)(c >> 8);
    dmaCopyCGram(backdrop_pal, PAL_BACKDROP << 4, 32);
    /* Every cast member's portrait, so walking on costs no upload. */
    for (i = 0; i < scene_cast_count(s_scene) && i < 4; i++)
        art_upload(ART_CAST, scene_cast_portrait(s_scene, i),
            PORTRAIT_TILE + i * PORTRAIT_TILES, PAL_STAGE, PORTRAIT_TILES * 32);
    bgSetGfxPtr(1, VRAM_BG1_TILES);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    bgSetScroll(1, 0, 0);
    draw_backdrop(scene_backdrop(s_scene));
    bgSetEnable(1);
    bg1_frame(1, BOX_Y, 30, BOX_H);
    stage_clear(&s_stage);
    for (i = 0; i < STAGE_SIZE; i++)
        s_slot_who[i] = STORY_NONE;
    dialogue_start(&s_dlg, g_story_scene, g_story_flags);
    show_step();
    video_commit();  /* the first speaker's palette, still in forced blank */
}

/* Back to the board's layers and palettes (main.c, in forced blank). */
void dialogue_leave(void)
{
    bgSetDisable(1);
    video_restore_palette();
}

static void finish(void)
{
    scene_goto(g_story_next);
}

void dialogue_update(void)
{
    if (s_step.kind == STEP_END) {
        finish();
        return;
    }
    if (g_pressed & KEY_START) {
        input_swallow();
        finish();
        return;
    }
    if (s_step.kind == STEP_LINE) {
        if (s_typing) {
            if (g_pressed & (KEY_A | KEY_B))
                type_more(255);
            else
                type_more(TYPE_SPEED);
            return;
        }
        if (g_pressed & (KEY_A | KEY_B)) {
            dialogue_advance(&s_dlg);
            show_step();
        }
        return;
    }
    /* Choices. */
    {
        u8 old = s_choice;
        if (menu_move(&s_choice, s_step.choices)) {
            const u8 *node = scene_node(s_scene, s_dlg.node);
            menu_pointer(TEXT_X, TEXT_Y + old, 0);
            menu_pointer(TEXT_X, TEXT_Y + s_choice, 1);
            bg3_text_n(TEXT_X + 2, TEXT_Y + old,
                (const char *)story_string(CHOICE_TEXT(node_choice(node, s_step.choice[old]))),
                TEXT_W - 2, TEXT_INK);
            bg3_text_n(TEXT_X + 2, TEXT_Y + s_choice,
                (const char *)story_string(CHOICE_TEXT(node_choice(node, s_step.choice[s_choice]))),
                TEXT_W - 2, TEXT_SELECTED);
        }
        if (g_pressed & KEY_A) {
            dialogue_choose(&s_dlg, s_step.choice[s_choice]);
            show_step();
        }
    }
}
