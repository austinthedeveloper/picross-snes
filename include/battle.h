#ifndef BATTLE_H
#define BATTLE_H

#include "types.h"
#include "puzzle.h"

/* Fight rules (ADR-010): a line-for-line port of the web game's
 * src/game/battle.ts and the engine helpers it uses (swap.ts, fakes.ts,
 * ink.ts, charge.ts, blast.ts, transform.ts). Function names follow the web
 * ones; comments say where they differ and why.
 *
 * Lines: a line id is a row index (0..19) or LINE_COL + a column index.
 * Line sets are flags per line (g_battle.line[id]), so a set operation is a
 * byte test. Ordered lists (phase pools) are count-prefixed byte strings of
 * line ids, in priority order, straight from ROM. */

#define LINE_COL 20
#define LINE_COUNT 40
#define LINE_NONE 0xFF
#define LINE_IS_COL(id) ((id) >= LINE_COL)
#define LINE_INDEX(id) ((id) >= LINE_COL ? (id) - LINE_COL : (id))

/* Per-line flags: which of the battle's line sets hold the line. */
enum {
    LF_FOG = 1,
    LF_SCRAMBLE = 2,
    LF_ARMOR = 4,     /* a weak point not yet solved */
    LF_FAKE = 8,      /* a forged clue not yet called out */
    LF_SUSPECT = 16,  /* marked as maybe forged */
    LF_WIPED = 32,    /* the Mimic wiped it */
    LF_STRUCK = 64,   /* has dealt its chip damage */
    LF_DONE = 128     /* complete as the player sees it (the view's g.complete) */
};

/* Fight counts (web FightCounts, #107). */
enum {
    COUNT_WRONG_FILLS, COUNT_ZAPS, COUNT_WRONG_ACCUSATIONS, COUNT_GADGETS_USED,
    COUNT_BOMBS_USED, COUNT_SPONGES_USED, COUNT_KINDS
};

/* Gadget effects (web GadgetEffect). */
enum { EFFECT_BLAST = 0, EFFECT_SHIELD = 1, EFFECT_WASH = 2 };

/* A boss phase (web Phase), decoded from ROM. Pools point at ROM lists. */
typedef struct {
    /* Starts once enemy HP <= at_hp: floor(at * hp), worked out by the
     * generator with the same double arithmetic as the web's `at * hp`. */
    u8 at_hp;
    u8 attack;          /* PHASE_KEEP: unchanged */
    u8 sprite;          /* PHASE_KEEP: unchanged */
    u8 fog_count;       /* PHASE_ALL: the whole pool */
    u8 scramble_count;
    u8 swap;
    u8 ink;             /* 0: none */
    u8 ink_count;
    u8 charge;          /* 0: unchanged */
    u8 rotate;          /* 0: unchanged */
    const u8 *name;     /* NULL: unchanged */
    const u8 *taunt;    /* NULL: none */
    const u8 *fog;      /* NULL: none */
    const u8 *scramble;
} Phase;
#define PHASE_KEEP 0xFF
#define PHASE_ALL 0xFF
#define MAX_PHASES 8

typedef struct {
    const u8 *name;
    u8 sprite;
    u8 hp;
    u8 attack;
    u8 charge;          /* 0: none */
    u8 rotate;          /* 0: none */
    u8 armor_attack;
    u8 has_armor;
    u8 phase_count;
    const u8 *fog;      /* count-prefixed line lists; NULL: none */
    const u8 *scramble;
    const u8 *weak;
    const u8 *fakes;
    const u8 *suspects;
    Phase phases[MAX_PHASES];
} Enemy;

/* A fight in progress (web BattleState). Saved with the run. */
typedef struct {
    u8 enemy_hp;
    u8 phase;
    u8 shield;
    u8 live;            /* the live line, or LINE_NONE */
    u8 charged;
    u8 warm_up;         /* lines left before the charge first goes live */
    u8 warm_up_set;     /* warm_up has been set (web: warmUp !== undefined) */
    u8 armor_lines;
    u8 turns;           /* quarter turns clockwise, 0..3 */
    u8 turning;
    u8 counts[COUNT_KINDS];
    u8 line[LINE_COUNT];
    /* Inked cells, one bit each, cell i at ink[i >> 3] bit (i & 7). */
    u8 ink[MAX_CELLS / 8];
} BattleState;

extern Enemy g_enemy;
extern BattleState g_battle;

/* Tuning constants come from the web (story_const.h, generated). */
#include "story_const.h"

/* What the enemy is now (web currentForm). */
typedef struct {
    const u8 *name;
    u8 sprite;
    u8 attack;
    u8 charge;
    u8 rotate;
    u8 armored;
} Form;
void battle_form(Form *out);

void battle_start(void);
u8 battle_armor_up(void);
u8 battle_count(u8 flag);
u8 line_has(u8 id, u8 flag);

/* web advancePhase: starts every phase the HP has fallen into, adding
 * fog and scramble from their pools (turned with the board). Returns how
 * many phases started; they are g_enemy.phases[first .. first + n - 1]
 * with first the phase index before the call. */
u8 battle_advance_phase(u8 size);

/* What completing lines did (web completeLines' pieces). */
typedef struct {
    u8 fog_revealed;
    u8 ink_washed;
    u8 unwiped;
    u8 honest;          /* suspects that checked out */
    u8 unscrambled;
    u8 dealt;
    u8 cracked;
    u8 broke;
    u8 phases;          /* phases entered */
    u8 phase_first;
} LinesResult;

/* web completeLines (the rules half of fight/lines.ts): lines are the
 * newly complete lines that count (not faked), n of them. */
void battle_complete_lines(const u8 *lines, u8 n, u8 width, LinesResult *out);

/* web armorHitsBack: lines are this move's fresh lines; weak_before a copy
 * of which were weak points before they landed (LF_ARMOR per line id). */
u8 battle_armor_hits_back(const u8 *lines, u8 n, const u8 *weak_before);

/* web chargeUp. */
typedef struct { u8 grounded; u8 zapped; } ChargeResult;
void battle_charge_up(const u8 *lines, u8 n, u8 limit, ChargeResult *out);
u8 battle_warm_up_lines(u8 width, u8 height);

/* web turnUp / turnDue / turnBattle. */
void battle_turn_up(u8 n);
u8 battle_turn_due(u8 limit);
void battle_turn(u8 n);

/* web callOut: returns 1 when a forgery was caught; *dealt the damage. */
u8 battle_call_out(u8 id, u8 *dealt);
/* web heal (an honest clue accused in a Relaxed run). */
u8 battle_heal(void);

/* web useGadget: returns the damage dealt. value: blast damage or shield
 * hits. */
u8 battle_use_gadget(u8 effect, u8 value);
/* web absorbHit: 1 when a shield blocked it. */
u8 battle_absorb_hit(void);
/* web finish: the finisher's damage. */
u8 battle_finish(void);
void battle_bump(u8 count);

/* Ink bits. */
u8 ink_at(u16 cell);
void ink_set(u16 cell, u8 on);
u16 ink_total(void);

/* Engine helpers (the web src/engine files). Boards are g_cells and g_solution. */

/* web satisfiedClues(...).every(Boolean): the line is complete as shown.
 * clue/count: the clue shown (a forgery for a faked line). */
u8 line_satisfied(u8 id, const u8 *clue, u8 count);

/* web fakeClue: writes the forged clue for clue[0..count) into out and
 * returns its length, or 0xFF if none fits (the generator rejects that). */
u8 fake_clue(const u8 *clue, u8 count, u8 length, u8 *out);

/* web pickSwap: returns 1 and the two lines (sorted) when a swap would
 * cost something. The cells to wipe are every non-empty cell in them. */
u8 pick_swap(u8 *a, u8 *b);

/* web pickInk: inks the best size x size block; returns cells inked. */
u16 pick_ink(u8 size);

/* web pickLiveLine: the line to go live, or LINE_NONE. Skips struck lines
 * and lines you can't read (fog, scramble, suspects). */
u8 pick_live_line(void);

/* web blastSize / pickBlast. rand: 0..65535. Returns 0 if nothing is left
 * to reveal; else the top-left and size of the blast. */
u8 blast_size(u8 width, u8 height);
u8 pick_blast(u8 size, u16 rand, u8 *top, u8 *left);

/* web turnLine: where a line goes when an n x n board turns clockwise. */
u8 turn_line(u8 id, u8 n);
u16 turn_index(u16 i, u8 n);

#endif
