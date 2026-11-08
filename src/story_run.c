#include "story_run.h"
#include "battle.h"
#include "play.h"
#include "story.h"

/* Ports of src/game/run.ts, runSave.ts and the jobs board's rules. */

JobStats *job_stats(u8 r)
{
    return &g_save.story.jobs[run_slot(r)];
}

u8 job_unlocked(u8 r)
{
    u8 need = run_requires(r);

    if (need == STORY_NONE)
        return 1;
    return job_stats(need)->wins != 0;
}

u8 job_cleared(u8 r)
{
    return job_stats(r)->wins != 0;
}

u8 chapter_open(u8 c)
{
    return job_unlocked(chapter_job(c, 0));
}

u8 scene_seen(u8 s)
{
    u8 slot = scene_slot(s);

    return (g_save.story.seen_scenes[slot >> 3] >> (slot & 7)) & 1;
}

void scene_mark_seen(u8 s)
{
    u8 slot = scene_slot(s);

    g_save.story.seen_scenes[slot >> 3] |= (u8)(1 << (slot & 7));
}

u8 run_current(void)
{
    if (g_run.slot1 == 0)
        return STORY_NONE;
    return run_for_slot(g_run.slot1 - 1);
}

void run_clear(void)
{
    u8 *p = (u8 *)&g_run;
    u16 i;

    for (i = 0; i < sizeof(RunSave); i++)
        p[i] = 0;
}

/* web startRun */
void run_start(u8 r, u8 mode)
{
    u8 i;

    run_clear();
    g_run.slot1 = run_slot(r) + 1;
    g_run.hp = run_hp(r);
    g_run.max_hp = g_run.hp;
    g_run.status = RUN_ACTIVE;
    g_run.mode = mode;
    g_run.has_tally = 1;
    for (i = 0; i < run_gadget_count(r) && i < RUN_GADGET_CAP; i++)
        g_run.gadgets[i] = run_gadget(r, i) + 1;
}

/* web damage */
void run_damage(u8 amount)
{
    if (g_run.status != RUN_ACTIVE || amount == 0)
        return;
    if (amount >= g_run.hp) {
        g_run.hp = 0;
        g_run.status = RUN_LOST;
        return;
    }
    g_run.hp -= amount;
}

/* web heal */
void run_heal(u8 amount)
{
    u16 hp;

    if (g_run.status != RUN_ACTIVE)
        return;
    hp = (u16)g_run.hp + amount;
    g_run.hp = hp > g_run.max_hp ? g_run.max_hp : (u8)hp;
}

/* web completeEncounter: clears the board and battle and moves on. */
void run_complete_encounter(void)
{
    u8 r = run_current();

    if (g_run.status != RUN_ACTIVE)
        return;
    g_run.node++;
    g_run.board_on = 0;
    g_run.battle_on = 0;
    if (r != STORY_NONE && g_run.node >= run_length(r))
        g_run.status = RUN_WON;
}

u8 run_gadget_add(u8 g)
{
    u8 i;

    for (i = 0; i < RUN_GADGET_CAP; i++) {
        if (g_run.gadgets[i] == 0) {
            g_run.gadgets[i] = g + 1;
            return 1;
        }
    }
    return 0;
}

/* Held gadgets stay packed at the front, in the order they came. */
void run_gadget_remove(u8 i)
{
    for (; i + 1 < RUN_GADGET_CAP; i++)
        g_run.gadgets[i] = g_run.gadgets[i + 1];
    g_run.gadgets[RUN_GADGET_CAP - 1] = 0;
}

u8 run_gadget_count_held(void)
{
    u8 i;

    for (i = 0; i < RUN_GADGET_CAP && g_run.gadgets[i]; i++)
        ;
    return i;
}

/* web addToTally */
void run_add_tally(const u8 *counts, u8 hints)
{
    u8 i;
    u16 v;

    if (g_run.has_tally == 0)
        return;
    for (i = 0; i < COUNT_KINDS; i++) {
        v = (u16)g_run.tally[i] + counts[i];
        g_run.tally[i] = v > 255 ? 255 : (u8)v;
    }
    v = (u16)g_run.tally[6] + hints;
    g_run.tally[6] = v > 255 ? 255 : (u8)v;
}

/* web recordRunResult (jobs; Endless's depth is M5). */
void run_record_result(void)
{
    JobStats *j;
    u8 r = run_current();
    u8 best;

    if (r == STORY_NONE || g_run.status == RUN_ACTIVE || g_run.recorded)
        return;
    j = job_stats(r);
    if (j->plays < 255)
        j->plays++;
    if (g_run.status == RUN_WON) {
        if (j->wins < 255)
            j->wins++;
        if (g_run.mode == MODE_HEARTS) {
            /* A first Hits-on win starts the best fresh. */
            best = j->hits_on_wins ? j->best_hp : 0;
            j->best_hp = g_run.hp > best ? g_run.hp : best;
            if (j->hits_on_wins < 255)
                j->hits_on_wins++;
        }
    }
    g_run.recorded = 1;
}
