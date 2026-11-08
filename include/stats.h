#ifndef STATS_H
#define STATS_H

#include "save.h"

/* Everything the records screens show, worked out from the save in one
 * pass (ADR-004). Nothing here is stored, so a save always shows the right
 * rank, badges and achievements, with no migration. */

enum { TIER_NONE = 0, TIER_BRONZE = 1, TIER_SILVER = 2, TIER_GOLD = 3 };
enum { SIZE_5 = 0, SIZE_10 = 1, SIZE_15 = 2, SIZE_20 = 3 };

#define RANK_COUNT 10
#define ACHIEVEMENT_COUNT 19

typedef struct {
    u16 solved;
    u16 solved_by_size[4];
    u16 points;
    u16 hearts_puzzles;
    u16 perfect_puzzles;
    u16 hours;
    u8 nohint_big;
    u8 fast_big;
    u8 rank;
    u8 badges;
    u8 golds;
    u8 tiers[COLLECTION_COUNT];
    u8 collection_solved[COLLECTION_COUNT];
} Stats;

extern Stats g_stats;

void stats_compute(Stats *s);
u8 stats_size_index(u8 side);

/* Ranks (web: src/puzzles/ranks.ts). */
const char *rank_name(u8 rank);
u16 rank_points(u8 rank);
u8 rank_for(u16 points);

/* Badges: one per collection. Bronze: every puzzle solved. Silver: every
 * puzzle solved in hearts mode. Gold: every puzzle solved perfectly. */
const char *tier_name(u8 tier);

/* Achievements. */
const char *achievement_name(u8 id);
/* Up to two lines, split at '\n'. */
const char *achievement_text(u8 id);
u16 achievement_target(u8 id);
u16 achievement_value(u8 id, const Stats *s);
u8 achievement_done(u8 id, const Stats *s);
u8 achievements_done(const Stats *s);

#endif
