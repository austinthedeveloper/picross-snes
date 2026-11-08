#ifndef STORY_H
#define STORY_H

#include "types.h"
#include "story_const.h"

/* The story tables in ROM (tools/gen_story.py has the layout; ADR-010).
 * Indexes are table indexes (encounter e, run r, ...), not save slots;
 * run_slot() gives a job's save slot. 0xFF means "none" for one-byte
 * references, 0xFFFF for strings. */

#define STORY_NONE 0xFF
#define STORY_NO_STRING 0xFFFF

enum { ENC_FIGHT = 0, ENC_DIG = 1, ENC_REST = 2 };

void story_init(void);

/* A string from the text banks, NUL-terminated (glyph codes 0x80+), or
 * NULL for STORY_NO_STRING. */
const u8 *story_string(u16 id);

/* Chapters, in order. */
u8 chapter_order(u8 c);
u8 chapter_opening(u8 c);
u16 chapter_title(u8 c);
u16 chapter_blurb(u8 c);
u8 chapter_job_count(u8 c);
u8 chapter_job(u8 c, u8 i);

/* Runs: jobs, Overtime, boss fights. */
u8 run_slot(u8 r);
u8 run_hp(u8 r);
u8 run_order(u8 r);
u8 run_requires(u8 r);
u8 run_overtime_of(u8 r);
u8 run_boss_of(u8 r);
u8 run_intro(u8 r);
u8 run_outro(u8 r);
u16 run_name(u8 r);
u16 run_blurb(u8 r);
u8 run_length(u8 r);
u8 run_encounter(u8 r, u8 i);
u8 run_gadget_count(u8 r);
u8 run_gadget(u8 r, u8 i);
/* The run with this save slot, or STORY_NONE. */
u8 run_for_slot(u8 slot);

/* Encounters. */
u8 enc_kind(u8 e);
u8 enc_boss(u8 e);
u8 enc_scene(u8 e);
u8 enc_rest_seconds(u8 e);
u8 enc_rest_heal(u8 e);
u8 enc_gadget(u8 e);
u16 enc_puzzle(u8 e);
u16 enc_name(u8 e);
u16 enc_blurb(u8 e);
/* Fills g_enemy from a fight encounter's enemy (battle.h). */
void story_load_enemy(u8 e);

/* Gadgets. */
u8 gadget_effect(u8 g);
u8 gadget_value(u8 g);
u8 gadget_icon(u8 g);
u16 gadget_name(u8 g);
u16 gadget_description(u8 g);

/* Scenes: a record pointer, read with the scene_* helpers. */
const u8 *story_scene(u8 s);
u8 scene_backdrop(const u8 *scene);
u8 scene_cast_count(const u8 *scene);
u8 scene_cast_portrait(const u8 *scene, u8 who);
u16 scene_cast_name(const u8 *scene, u8 who);
u8 scene_start(const u8 *scene);
const u8 *scene_node(const u8 *scene, u8 node);
/* A node: next, lines (who, cond, text), choices (next, cond, set mask,
 * set values, text). */
#define NODE_NEXT(n) ((n)[0])
#define NODE_LINES(n) ((n)[1])
const u8 *node_line(const u8 *node, u8 i);          /* 4 bytes */
u8 node_choice_count(const u8 *node);
const u8 *node_choice(const u8 *node, u8 i);        /* 6 bytes */
#define LINE_WHO(l) ((l)[0])
#define LINE_COND(l) ((l)[1])
#define LINE_TEXT(l) ((u16)((l)[2] | ((l)[3] << 8)))
#define CHOICE_NEXT(c) ((c)[0])
#define CHOICE_COND(c) ((c)[1])
#define CHOICE_SET_MASK(c) ((c)[2])
#define CHOICE_SET_VALUES(c) ((c)[3])
#define CHOICE_TEXT(c) ((u16)((c)[4] | ((c)[5] << 8)))

/* Backdrops: width, height, then rows of bits. */
const u8 *story_backdrop(u8 b);

#endif
