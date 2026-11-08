#include "story.h"
#include "battle.h"
#include "story_banks.h"

/* Reads the story tables (tools/gen_story.py, ADR-010). */

static const u8 *s_text[STORY_TEXT_BANKS];

enum { SEC_CHAPTERS, SEC_RUNS, SEC_ENCOUNTERS, SEC_SCENES, SEC_GADGETS, SEC_BACKDROPS };

static u16 read16(const u8 *p)
{
    return (u16)(p[0] | (p[1] << 8));
}

static const u8 *tab(void)
{
    return ROM_DATA(story_tab);
}

void story_init(void)
{
    STORY_TEXT_BANK_INIT(s_text);
}

/* The record of item i in a section. */
static const u8 *record(u8 section, u8 i)
{
    const u8 *t = tab();
    u16 index = read16(t + ((u16)section << 2) + 2);

    return t + read16(t + index + ((u16)i << 1));
}

static const u8 *tab_at(u16 off)
{
    return off ? tab() + off : 0;
}

const u8 *story_string(u16 id)
{
    const u8 *e;

    if (id == STORY_NO_STRING)
        return 0;
    e = ROM_DATA(story_str) + id + id + id;
    return s_text[e[0]] + read16(e + 1);
}

/* chapter: order, opening, u16 title, u16 blurb, n, jobs[n] */
u8 chapter_order(u8 c) { return record(SEC_CHAPTERS, c)[0]; }
u8 chapter_opening(u8 c) { return record(SEC_CHAPTERS, c)[1]; }
u16 chapter_title(u8 c) { return read16(record(SEC_CHAPTERS, c) + 2); }
u16 chapter_blurb(u8 c) { return read16(record(SEC_CHAPTERS, c) + 4); }
u8 chapter_job_count(u8 c) { return record(SEC_CHAPTERS, c)[6]; }
u8 chapter_job(u8 c, u8 i) { return record(SEC_CHAPTERS, c)[7 + i]; }

/* run: slot, hp, order, requires, overtimeOf, bossOf, intro, outro,
 * u16 name, u16 blurb, n, encounters[n], g, gadgets[g] */
u8 run_slot(u8 r) { return record(SEC_RUNS, r)[0]; }
u8 run_hp(u8 r) { return record(SEC_RUNS, r)[1]; }
u8 run_order(u8 r) { return record(SEC_RUNS, r)[2]; }
u8 run_requires(u8 r) { return record(SEC_RUNS, r)[3]; }
u8 run_overtime_of(u8 r) { return record(SEC_RUNS, r)[4]; }
u8 run_boss_of(u8 r) { return record(SEC_RUNS, r)[5]; }
u8 run_intro(u8 r) { return record(SEC_RUNS, r)[6]; }
u8 run_outro(u8 r) { return record(SEC_RUNS, r)[7]; }
u16 run_name(u8 r) { return read16(record(SEC_RUNS, r) + 8); }
u16 run_blurb(u8 r) { return read16(record(SEC_RUNS, r) + 10); }
u8 run_length(u8 r) { return record(SEC_RUNS, r)[12]; }
u8 run_encounter(u8 r, u8 i) { return record(SEC_RUNS, r)[13 + i]; }

u8 run_gadget_count(u8 r)
{
    const u8 *p = record(SEC_RUNS, r);

    return p[13 + p[12]];
}

u8 run_gadget(u8 r, u8 i)
{
    const u8 *p = record(SEC_RUNS, r);

    return p[14 + p[12] + i];
}

u8 run_for_slot(u8 slot)
{
    u8 r;

    for (r = 0; r < STORY_RUNS; r++) {
        if (run_slot(r) == slot)
            return r;
    }
    return STORY_NONE;
}

/* encounter: kind, boss, scene, rest seconds, rest heal, gadget,
 * u16 puzzle, u16 name, u16 blurb, has enemy, 0; enemy at +14 */
u8 enc_kind(u8 e) { return record(SEC_ENCOUNTERS, e)[0]; }
u8 enc_boss(u8 e) { return record(SEC_ENCOUNTERS, e)[1]; }
u8 enc_scene(u8 e) { return record(SEC_ENCOUNTERS, e)[2]; }
u8 enc_rest_seconds(u8 e) { return record(SEC_ENCOUNTERS, e)[3]; }
u8 enc_rest_heal(u8 e) { return record(SEC_ENCOUNTERS, e)[4]; }
u8 enc_gadget(u8 e) { return record(SEC_ENCOUNTERS, e)[5]; }
u16 enc_puzzle(u8 e) { return read16(record(SEC_ENCOUNTERS, e) + 6); }
u16 enc_name(u8 e) { return read16(record(SEC_ENCOUNTERS, e) + 8); }
u16 enc_blurb(u8 e) { return read16(record(SEC_ENCOUNTERS, e) + 10); }

/* enemy: sprite, hp, attack, charge, rotate, armor attack, has armor,
 * phases, u16 name, u16 fog, scramble, weak, fakes, suspects; then 18
 * bytes a phase: at_hp, attack, sprite, fog count, scramble count, swap,
 * ink, ink count, charge, rotate, u16 name, u16 taunt, u16 fog,
 * u16 scramble */
void story_load_enemy(u8 e)
{
    const u8 *p = record(SEC_ENCOUNTERS, e) + 14;
    u8 i;
    Phase *ph;

    g_enemy.sprite = p[0];
    g_enemy.hp = p[1];
    g_enemy.attack = p[2];
    g_enemy.charge = p[3];
    g_enemy.rotate = p[4];
    g_enemy.armor_attack = p[5];
    g_enemy.has_armor = p[6];
    g_enemy.phase_count = p[7];
    g_enemy.name = story_string(read16(p + 8));
    g_enemy.fog = tab_at(read16(p + 10));
    g_enemy.scramble = tab_at(read16(p + 12));
    g_enemy.weak = tab_at(read16(p + 14));
    g_enemy.fakes = tab_at(read16(p + 16));
    g_enemy.suspects = tab_at(read16(p + 18));
    p += 20;
    for (i = 0; i < g_enemy.phase_count; i++) {
        ph = &g_enemy.phases[i];
        ph->at_hp = p[0];
        ph->attack = p[1];
        ph->sprite = p[2];
        ph->fog_count = p[3];
        ph->scramble_count = p[4];
        ph->swap = p[5];
        ph->ink = p[6];
        ph->ink_count = p[7];
        ph->charge = p[8];
        ph->rotate = p[9];
        ph->name = story_string(read16(p + 10));
        ph->taunt = story_string(read16(p + 12));
        ph->fog = tab_at(read16(p + 14));
        ph->scramble = tab_at(read16(p + 16));
        p += 18;
    }
}

/* gadget: effect, value, icon, u16 name, u16 description */
u8 gadget_effect(u8 g) { return record(SEC_GADGETS, g)[0]; }
u8 gadget_value(u8 g) { return record(SEC_GADGETS, g)[1]; }
u8 gadget_icon(u8 g) { return record(SEC_GADGETS, g)[2]; }
u16 gadget_name(u8 g) { return read16(record(SEC_GADGETS, g) + 3); }
u16 gadget_description(u8 g) { return read16(record(SEC_GADGETS, g) + 5); }

/* scene: backdrop, cast n, (portrait, u16 name)[n], start, nodes m,
 * u16 node offsets[m] (from the scene record) */
const u8 *story_scene(u8 s) { return record(SEC_SCENES, s); }
u8 scene_backdrop(const u8 *scene) { return scene[0]; }
u8 scene_cast_count(const u8 *scene) { return scene[1]; }
u8 scene_cast_portrait(const u8 *scene, u8 who) { return scene[2 + who * 3]; }
u16 scene_cast_name(const u8 *scene, u8 who) { return read16(scene + 3 + who * 3); }

u8 scene_start(const u8 *scene)
{
    return scene[2 + scene[1] * 3];
}

const u8 *scene_node(const u8 *scene, u8 node)
{
    const u8 *index = scene + 4 + scene[1] * 3;

    return scene + read16(index + ((u16)node << 1));
}

const u8 *node_line(const u8 *node, u8 i)
{
    return node + 2 + ((u16)i << 2);
}

u8 node_choice_count(const u8 *node)
{
    return node[2 + ((u16)node[1] << 2)];
}

const u8 *node_choice(const u8 *node, u8 i)
{
    return node + 3 + ((u16)node[1] << 2) + i * 6;
}

const u8 *story_backdrop(u8 b)
{
    return record(SEC_BACKDROPS, b);
}
