#ifndef STORY_RUN_H
#define STORY_RUN_H

#include "types.h"
#include "save.h"

/* Story progress (ADR-010): ports of the web game's src/game/run.ts
 * (startRun, damage, heal, completeEncounter, gadgets, the tally),
 * runSave.ts (recordRunResult, seen scenes) and the jobs board's unlock
 * rules. Everything lives in g_save.story; callers save_write() at safe
 * points. r is a run's table index (story.h), not its save slot. */

#define g_run (g_save.story.run)

JobStats *job_stats(u8 r);
/* web: a job with `requires` is locked until that job is won once. */
u8 job_unlocked(u8 r);
u8 job_cleared(u8 r);
/* A chapter opens once its first job does. */
u8 chapter_open(u8 c);

u8 scene_seen(u8 s);
void scene_mark_seen(u8 s);

/* The run in progress, or STORY_NONE. */
u8 run_current(void);
void run_start(u8 r, u8 mode);
void run_clear(void);
/* web damage: the run is lost at 0 HP. */
void run_damage(u8 amount);
void run_heal(u8 amount);
/* web completeEncounter: moves on; won after the last one. */
void run_complete_encounter(void);
u8 run_gadget_add(u8 g);
/* Removes the i-th held gadget. */
void run_gadget_remove(u8 i);
u8 run_gadget_count_held(void);
/* web addToTally: a won fight's counts (battle.h COUNT_*) and hints. */
void run_add_tally(const u8 *counts, u8 hints);
/* web recordRunResult: once per finished run. */
void run_record_result(void);

#endif
