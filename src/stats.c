#include "puzzle.h"
#include "progress.h"
#include "stats.h"

Stats g_stats;

/* A 15x15 or bigger board in under five minutes. */
#define FAST_SECONDS 300

static const char *const s_rank_names[RANK_COUNT] = {
    "HOBBYIST", "TINKERER", "ODD-JOBBER", "FIXER", "LICENSED FIXER",
    "SPECIALIST", "EXPERT", "LOCAL LEGEND", "REGIONAL LEGEND",
    "NO JOB TOO STRANGE"
};
static const u16 s_rank_points[RANK_COUNT] = {
    0, 10, 30, 75, 150, 300, 500, 750, 1100, 1500
};

const char *rank_name(u8 rank)
{
    return s_rank_names[rank];
}

u16 rank_points(u8 rank)
{
    return s_rank_points[rank];
}

u8 rank_for(u16 points)
{
    u8 r = RANK_COUNT - 1;

    while (r && points < s_rank_points[r])
        r--;
    return r;
}

static const char *const s_tier_names[4] = {
    "NOT YET", "BRONZE", "SILVER", "GOLD"
};

const char *tier_name(u8 tier)
{
    return s_tier_names[tier];
}

/* Per collection: puzzles solved in hearts mode, and perfectly. */
static u8 s_hearts[COLLECTION_COUNT];
static u8 s_perfect[COLLECTION_COUNT];

void stats_compute(Stats *s)
{
    /* One walk over the info table and the records together: 816-tcc makes
     * a call per puzzle cost too much here (a solve shows this). */
    const u8 *info = pack_info_table();
    const Record *rec = g_save.records;
    u16 slot;
    u8 flags;
    u8 side;
    u8 c;
    u8 n;

    s->solved = 0;
    s->points = 0;
    s->hearts_puzzles = 0;
    s->perfect_puzzles = 0;
    s->nohint_big = 0;
    s->fast_big = 0;
    s->badges = 0;
    s->golds = 0;
    s->solved_by_size[0] = 0;
    s->solved_by_size[1] = 0;
    s->solved_by_size[2] = 0;
    s->solved_by_size[3] = 0;
    for (c = 0; c < COLLECTION_COUNT; c++) {
        s->collection_solved[c] = 0;
        s_hearts[c] = 0;
        s_perfect[c] = 0;
    }
    s->tiles_solved = 0;
    for (slot = 0; slot < PUZZLE_COUNT; slot++) {
        flags = rec->flags;
        c = info[PACK_INFO_COLLECTION];
        if ((flags & REC_SOLVED) && c == MURAL_COLLECTION) {
            /* A mural tile: rank points (the web game's 3) and the murals. */
            s->tiles_solved++;
            s->points += 3;
        } else if (flags & REC_SOLVED) {
            side = info[PACK_INFO_WIDTH];
            s->solved++;
            s->collection_solved[c]++;
            if (side <= 5) {
                s->solved_by_size[SIZE_5]++;
                s->points += 1;
            } else if (side <= 10) {
                s->solved_by_size[SIZE_10]++;
                s->points += 2;
            } else if (side <= 15) {
                s->solved_by_size[SIZE_15]++;
                s->points += 3;
            } else {
                s->solved_by_size[SIZE_20]++;
                s->points += 6;
            }
            if (flags & REC_HEARTS) {
                s->hearts_puzzles++;
                s_hearts[c]++;
            }
            if (flags & REC_PERFECT) {
                s->perfect_puzzles++;
                s_perfect[c]++;
            }
            if (side >= 15) {
                if (flags & REC_NOHINT)
                    s->nohint_big = 1;
                if (rec->best < FAST_SECONDS)
                    s->fast_big = 1;
            }
        }
        info += 8;
        rec++;
    }
    for (c = 0; c < COLLECTION_COUNT; c++) {
        n = collection_size(c);
        if (s_perfect[c] == n)
            s->tiers[c] = TIER_GOLD;
        else if (s_hearts[c] == n)
            s->tiers[c] = TIER_SILVER;
        else if (s->collection_solved[c] == n)
            s->tiers[c] = TIER_BRONZE;
        else
            s->tiers[c] = TIER_NONE;
        if (s->tiers[c])
            s->badges++;
        if (s->tiers[c] == TIER_GOLD)
            s->golds++;
    }
    s->murals_done = 0;
    for (c = 0; c < MURAL_COUNT; c++) {
        u16 first = mural_first_slot(c);
        u8 count = (u8)(mural_across(c) * mural_down(c));
        u8 got = 0;

        rec = g_save.records + first;
        for (n = 0; n < count; n++) {
            if (rec->flags & REC_SOLVED)
                got++;
            rec++;
        }
        s->mural_solved[c] = got;
        if (got == count)
            s->murals_done++;
    }
    s->rank = rank_for(s->points);
    s->hours = progress_hours();
}

/* ---------------------------------------------------------- achievements */

enum {
    ACH_SOLVED, ACH_SIZE20, ACH_ALL_SIZES, ACH_PERFECT, ACH_HEARTS,
    ACH_NOHINT_BIG, ACH_FAST_BIG, ACH_HOURS, ACH_BADGES, ACH_GOLDS, ACH_RANK,
    ACH_MURALS
};

static const char *const s_ach_names[ACHIEVEMENT_COUNT] = {
    "FIRST PICTURE", "WARMING UP", "HALF CENTURY", "CENTURION",
    "SKETCHBOOK", "THE WHOLE PACK", "BIG BOARD", "ALL SIZES",
    "CLEAN SHEET", "FLAWLESS TEN", "HEARTS AND MINDS", "NO PEEKING",
    "QUICK STUDY", "NIGHT SHIFT", "FIRST MEDAL", "SHOWCASE",
    "GOLD STANDARD", "LOCAL LEGEND", "NO JOB TOO STRANGE", "MURALIST",
    "THE GALLERY"
};
static const char *const s_ach_text[ACHIEVEMENT_COUNT] = {
    "SOLVE YOUR FIRST PUZZLE.",
    "SOLVE 10 PUZZLES.",
    "SOLVE 50 PUZZLES.",
    "SOLVE 100 PUZZLES.",
    "SOLVE 250 PUZZLES.",
    "SOLVE EVERY PUZZLE\nIN THE PACK.",
    "SOLVE A 20X20 PUZZLE.",
    "SOLVE A 5X5, 10X10,\n15X15 AND 20X20.",
    "SOLVE A PUZZLE IN HEARTS MODE\nWITH NO MISTAKES OR HINTS.",
    "SOLVE 10 PUZZLES PERFECTLY\nIN HEARTS MODE.",
    "SOLVE 25 DIFFERENT PUZZLES\nIN HEARTS MODE.",
    "SOLVE A 15X15 OR BIGGER\nWITHOUT A HINT.",
    "SOLVE A 15X15 OR BIGGER\nIN UNDER 5 MINUTES.",
    "PLAY FOR 10 HOURS.",
    "EARN A COLLECTION BADGE.",
    "EARN 10 COLLECTION BADGES.",
    "EARN A GOLD BADGE.",
    "REACH THE RANK OF\nLOCAL LEGEND.",
    "REACH THE TOP RANK.",
    "FINISH A MURAL.",
    "FINISH EVERY MURAL."
};
static const u8 s_ach_kind[ACHIEVEMENT_COUNT] = {
    ACH_SOLVED, ACH_SOLVED, ACH_SOLVED, ACH_SOLVED, ACH_SOLVED, ACH_SOLVED,
    ACH_SIZE20, ACH_ALL_SIZES, ACH_PERFECT, ACH_PERFECT, ACH_HEARTS,
    ACH_NOHINT_BIG, ACH_FAST_BIG, ACH_HOURS, ACH_BADGES, ACH_BADGES,
    ACH_GOLDS, ACH_RANK, ACH_RANK, ACH_MURALS, ACH_MURALS
};
static const u16 s_ach_target[ACHIEVEMENT_COUNT] = {
    1, 10, 50, 100, 250, PACK_LIVE_COUNT, 1, 4, 1, 10, 25, 1, 1, 10, 1, 10,
    1, 7, 9, 1, MURAL_COUNT
};

const char *achievement_name(u8 id)
{
    return s_ach_names[id];
}

const char *achievement_text(u8 id)
{
    return s_ach_text[id];
}

u16 achievement_target(u8 id)
{
    return s_ach_target[id];
}

u16 achievement_value(u8 id, const Stats *s)
{
    u8 i;
    u16 n = 0;

    switch (s_ach_kind[id]) {
    case ACH_SOLVED:
        return s->solved;
    case ACH_SIZE20:
        return s->solved_by_size[SIZE_20];
    case ACH_ALL_SIZES:
        for (i = 0; i < 4; i++)
            if (s->solved_by_size[i])
                n++;
        return n;
    case ACH_PERFECT:
        return s->perfect_puzzles;
    case ACH_HEARTS:
        return s->hearts_puzzles;
    case ACH_NOHINT_BIG:
        return s->nohint_big;
    case ACH_FAST_BIG:
        return s->fast_big;
    case ACH_HOURS:
        return s->hours;
    case ACH_BADGES:
        return s->badges;
    case ACH_GOLDS:
        return s->golds;
    case ACH_RANK:
        return s->rank;
    case ACH_MURALS:
        return s->murals_done;
    }
    return 0;
}

u8 achievement_done(u8 id, const Stats *s)
{
    return achievement_value(id, s) >= s_ach_target[id];
}

u8 achievements_done(const Stats *s)
{
    u8 id;
    u8 n = 0;

    for (id = 0; id < ACHIEVEMENT_COUNT; id++)
        if (achievement_done(id, s))
            n++;
    return n;
}
