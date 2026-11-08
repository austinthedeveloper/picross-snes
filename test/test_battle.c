/* Host tests for the fight rules (src/battle.c, src/fight_rules.c) and
 * the story tables (src/story.c), against the real imported story. They
 * follow the web game's src/game/battle.test.ts and engine tests. */
#include <string.h>

#include "battle.h"
#include "board.h"
#include "puzzle.h"
#include "story.h"
#include "test_support.h"

static u8 find_encounter(const char *name)
{
    u8 e;

    for (e = 0; e < STORY_ENCOUNTERS; e++) {
        if (strcmp((const char *)story_string(enc_name(e)), name) == 0)
            return e;
    }
    fprintf(stderr, "no encounter %s\n", name);
    exit(1);
}

static void start_fight(const char *name)
{
    u8 e = find_encounter(name);

    puzzle_load(enc_puzzle(e));
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    story_load_enemy(e);
    battle_start();
}

/* Fills a line from the solution, X-ing the gaps: completes it. */
static void solve_line(u8 id)
{
    u8 n = LINE_IS_COL(id) ? g_puzzle.height : g_puzzle.width;
    u8 k;
    u16 at;

    for (k = 0; k < n; k++) {
        at = LINE_IS_COL(id) ? g_row_start[k] + (id - LINE_COL) : g_row_start[id] + k;
        g_cells[at] = g_solution[at] ? CELL_FILLED : CELL_MARKED;
    }
}

static void complete(const u8 *lines, u8 n, LinesResult *r)
{
    battle_complete_lines(lines, n, g_puzzle.width, r);
}

static void test_tables(void)
{
    u8 e;
    u8 r;
    u8 c;
    u8 s;
    u8 i;
    u8 fights = 0;

    for (e = 0; e < STORY_ENCOUNTERS; e++) {
        CHECK(story_string(enc_name(e)) != 0);
        CHECK(enc_puzzle(e) < PACK_SAVE_SLOTS || enc_puzzle(e) >= STORY_PUZZLE_BASE);
        if (enc_kind(e) == ENC_FIGHT) {
            story_load_enemy(e);
            CHECK(g_enemy.hp > 0);
            CHECK(g_enemy.name != 0);
            CHECK(g_enemy.phase_count <= MAX_PHASES);
            fights++;
        }
        puzzle_load(enc_puzzle(e));
        CHECK(g_puzzle.width == g_puzzle.height);
        CHECK(g_puzzle.filled > 0);
    }
    CHECK_EQ(fights, 75);
    for (r = 0; r < STORY_RUNS; r++) {
        CHECK(run_length(r) > 0);
        for (i = 0; i < run_length(r); i++)
            CHECK(run_encounter(r, i) < STORY_ENCOUNTERS);
        CHECK_EQ(run_for_slot(run_slot(r)), r);
    }
    for (c = 0; c < STORY_CHAPTERS; c++) {
        CHECK(chapter_job_count(c) > 0);
        CHECK_EQ(chapter_order(c), c + 1);
    }
    CHECK(strcmp((const char *)story_string(chapter_title(0)), "Odd Jobs") == 0);
    /* Every scene line decodes to printable text. */
    for (s = 0; s < STORY_SCENES; s++) {
        const u8 *scene = story_scene(s);
        u8 nodes = scene[3 + scene[1] * 3];
        u8 n;
        for (n = 0; n < nodes; n++) {
            const u8 *node = scene_node(scene, n);
            for (i = 0; i < NODE_LINES(node); i++) {
                const u8 *line = node_line(node, i);
                const u8 *text = story_string(LINE_TEXT(line));
                CHECK(text != 0);
                for (; text && *text; text++)
                    CHECK((*text >= ' ' && *text <= '~') || *text >= 0x80);
                CHECK(LINE_WHO(line) == STORY_NONE || LINE_WHO(line) < scene_cast_count(scene));
            }
        }
    }
    /* First Job starts with a patch kit. */
    for (r = 0; r < STORY_RUNS; r++) {
        if (strcmp((const char *)story_string(run_name(r)), "First Job") == 0) {
            CHECK_EQ(run_gadget_count(r), 1);
            CHECK_EQ(run_gadget(r, 0), GADGET_PATCH_KIT);
            CHECK_EQ(run_hp(r), 12);
        }
    }
}

static void test_gremlin_phases(void)
{
    u8 lines[8];
    LinesResult res;
    Form form;
    u8 i;

    start_fight("Generator Gremlin");
    CHECK_EQ(g_enemy.hp, 45);
    CHECK_EQ(g_enemy.phases[0].at_hp, 40);  /* floor(0.9 * 45) */
    CHECK_EQ(g_enemy.phases[1].at_hp, 37);  /* floor(0.84 * 45) */
    CHECK(line_has(3, LF_FOG) && line_has(8, LF_FOG) && line_has(10, LF_FOG));
    CHECK_EQ(battle_count(LF_FOG), 3);

    /* Rows 0, 1, 2 and columns 0, 1: five lines, 5 damage, HP 40. */
    lines[0] = 0; lines[1] = 1; lines[2] = 2; lines[3] = LINE_COL; lines[4] = LINE_COL + 1;
    complete(lines, 5, &res);
    CHECK_EQ(res.dealt, 5);
    CHECK_EQ(g_battle.enemy_hp, 40);
    CHECK_EQ(res.phases, 1);
    battle_form(&form);
    CHECK_EQ(form.attack, 3);
    CHECK(strcmp((const char *)form.name, "Overcharged Gremlin") == 0);
    /* Pool [1, 7, 14, 4, 9], 3 of them: row 1 has struck, so 7, 14, 4. */
    CHECK(line_has(7, LF_FOG) && line_has(14, LF_FOG) && line_has(4, LF_FOG));
    CHECK(!line_has(1, LF_FOG) && !line_has(9, LF_FOG));
    CHECK_EQ(battle_count(LF_FOG), 6);

    /* The same lines again deal nothing. */
    complete(lines, 5, &res);
    CHECK_EQ(res.dealt, 0);

    /* Completing a fogged row reveals it. */
    lines[0] = 3;
    complete(lines, 1, &res);
    CHECK_EQ(res.fog_revealed, 1);
    CHECK(!line_has(3, LF_FOG));
    CHECK_EQ(g_battle.enemy_hp, 39);

    /* Two more to 37: the second phase fogs 2 more (9 is left in the pool). */
    lines[0] = LINE_COL + 2; lines[1] = LINE_COL + 3;
    complete(lines, 2, &res);
    CHECK_EQ(res.phases, 1);
    CHECK(line_has(9, LF_FOG));
    battle_form(&form);
    CHECK_EQ(form.attack, 4);

    /* Chip never drops it below 1; the finisher deals the rest. */
    for (i = 0; i < 8; i++)
        lines[i] = LINE_COL + 4 + i;
    complete(lines, 8, &res);
    for (i = 0; i < 3; i++)
        lines[i] = LINE_COL + 12 + i;
    complete(lines, 3, &res);
    for (i = 0; i < 8; i++)
        lines[i] = 5 + i;
    complete(lines, 8, &res);
    for (i = 0; i < 8; i++)
        lines[i] = 13 + i;
    complete(lines, 2, &res);
    CHECK(g_battle.enemy_hp >= 1);
    i = g_battle.enemy_hp;
    CHECK_EQ(battle_finish(), i);
    CHECK_EQ(g_battle.enemy_hp, 0);
}

static void test_armor(void)
{
    u8 lines[6];
    u8 weak[LINE_COUNT];
    LinesResult res;
    Form form;
    u8 hits;

    start_fight("Armored Beetle");
    CHECK(g_enemy.has_armor);
    battle_form(&form);
    CHECK(form.armored);
    CHECK_EQ(form.attack, 4);

    /* Five ordinary lines: no damage, and the armor hits back once. */
    lines[0] = 0; lines[1] = 1; lines[2] = 2; lines[3] = 3; lines[4] = 4;
    memcpy(weak, g_battle.line, LINE_COUNT);
    complete(lines, 5, &res);
    CHECK_EQ(res.dealt, 0);
    hits = battle_armor_hits_back(lines, 5, weak);
    CHECK_EQ(hits, 1);
    CHECK_EQ(g_battle.armor_lines, 0);

    /* A weak point: triple damage and a crack. */
    lines[0] = 8;
    memcpy(weak, g_battle.line, LINE_COUNT);
    complete(lines, 1, &res);
    CHECK_EQ(res.dealt, 3);
    CHECK_EQ(res.cracked, 1);
    CHECK(!res.broke);
    CHECK_EQ(battle_armor_hits_back(lines, 1, weak), 0);

    /* The last two break it; attack softens. */
    lines[0] = 11; lines[1] = LINE_COL + 2;
    complete(lines, 2, &res);
    CHECK_EQ(res.dealt, 6);
    CHECK(res.broke);
    battle_form(&form);
    CHECK(!form.armored);
    CHECK_EQ(form.attack, 2);
    /* Normal rules now. */
    lines[0] = 5;
    complete(lines, 1, &res);
    CHECK_EQ(res.dealt, 1);
}

static void test_fakes(void)
{
    u8 dealt;
    u8 clue[4];
    u8 out[MAX_SIDE];
    u8 n;

    start_fight("The Forger");
    CHECK(line_has(3, LF_FAKE) && line_has(3, LF_SUSPECT));
    CHECK(line_has(2, LF_SUSPECT) && !line_has(2, LF_FAKE));
    /* Accusing an honest suspect clears its mask and deals nothing. */
    CHECK_EQ(battle_call_out(2, &dealt), 0);
    CHECK_EQ(dealt, 0);
    CHECK(!line_has(2, LF_SUSPECT));
    /* A forgery is caught for 5. */
    CHECK_EQ(battle_call_out(3, &dealt), 1);
    CHECK_EQ(dealt, CALL_OUT_DAMAGE);
    CHECK_EQ(g_battle.enemy_hp, 45 - CALL_OUT_DAMAGE);
    CHECK(!line_has(3, LF_FAKE) && !line_has(3, LF_SUSPECT));
    /* A Relaxed false accusation heals, up to max. */
    CHECK_EQ(battle_heal(), 3);
    CHECK_EQ(battle_heal(), 2);

    /* web fakeClue examples: merge first, then split, lengthen, shorten. */
    clue[0] = 1; clue[1] = 2;
    n = fake_clue(clue, 2, 15, out);
    CHECK_EQ(n, 1);
    CHECK_EQ(out[0], 4);
    clue[0] = 3;
    n = fake_clue(clue, 1, 15, out);
    CHECK_EQ(n, 2);
    CHECK(out[0] == 1 && out[1] == 1);
    clue[0] = 2;
    n = fake_clue(clue, 1, 15, out);
    CHECK(n == 1 && out[0] == 3);
    clue[0] = 5;
    n = fake_clue(clue, 1, 5, out);
    CHECK(n == 2 && out[0] == 3 && out[1] == 1);
    n = fake_clue(clue, 0, 5, out);
    CHECK(n == 1 && out[0] == 1);
}

static void test_charge(void)
{
    u8 lines[8];
    ChargeResult cr;

    start_fight("Electric Eel");
    CHECK_EQ(g_enemy.charge, 5);
    CHECK_EQ(battle_warm_up_lines(15, 15), 8);  /* round(7.5) */
    CHECK_EQ(battle_warm_up_lines(10, 10), 5);
    CHECK_EQ(battle_warm_up_lines(20, 20), 10);
    CHECK_EQ(battle_warm_up_lines(5, 5), 3);    /* round(2.5) */
    g_battle.warm_up = 8;
    g_battle.warm_up_set = 1;
    lines[0] = 0; lines[1] = 1; lines[2] = 2;
    battle_charge_up(lines, 3, 5, &cr);
    CHECK_EQ(g_battle.warm_up, 5);
    CHECK(!cr.zapped && !cr.grounded);
    g_battle.warm_up = 0;
    g_battle.live = 6;
    battle_charge_up(lines, 3, 5, &cr);
    CHECK_EQ(g_battle.charged, 3);
    battle_charge_up(lines, 2, 5, &cr);
    CHECK(cr.zapped);
    CHECK_EQ(g_battle.charged, 0);
    CHECK_EQ(g_battle.live, LINE_NONE);
    g_battle.live = 6;
    g_battle.charged = 4;
    lines[0] = 6;
    battle_charge_up(lines, 1, 5, &cr);
    CHECK(cr.grounded && !cr.zapped);
    CHECK_EQ(g_battle.charged, 0);
}

static void test_line_satisfied(void)
{
    u8 clue[2];
    u16 base;

    puzzle_load(0);  /* the heart, 5x5 */
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    base = g_row_start[4];
    clue[0] = 1;
    /* "#....": the runs match exactly, so it's complete. */
    g_cells[base] = CELL_FILLED;
    CHECK(line_satisfied(4, clue, 1));
    /* "#..#." with clue 1: two runs, none locked in. */
    g_cells[base + 3] = CELL_FILLED;
    CHECK(!line_satisfied(4, clue, 1));
    g_cells[base + 3] = CELL_EMPTY;
    /* "#X..." locks it in from the left. */
    g_cells[base + 1] = CELL_MARKED;
    CHECK(line_satisfied(4, clue, 1));
    /* An extra run after a locked clue still counts, like the web. */
    g_cells[base + 3] = CELL_FILLED;
    CHECK(line_satisfied(4, clue, 1));
    /* Exact runs. */
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    clue[0] = 1; clue[1] = 1;
    g_cells[base + 1] = CELL_FILLED;
    g_cells[base + 3] = CELL_FILLED;
    CHECK(line_satisfied(4, clue, 2));
    /* An empty clue: complete while nothing is filled. */
    CHECK(line_satisfied(3, clue, 0));
    g_cells[g_row_start[3]] = CELL_FILLED;
    CHECK(!line_satisfied(3, clue, 0));
    /* A mistake X closes a run like a mark. */
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    clue[0] = 2;
    g_cells[base + 3] = CELL_FILLED;
    g_cells[base + 4] = CELL_FILLED;
    g_cells[base + 2] = CELL_MISTAKE;
    CHECK(line_satisfied(4, clue, 1));
}

static void test_pickers(void)
{
    u8 a = 0;
    u8 b = 0;
    u8 top = 0;
    u8 left = 0;

    start_fight("Mimic");
    /* Nothing marked: no swap. */
    CHECK(!pick_swap(&a, &b));
    /* Correct marks in rows 2 and 5 (partly): they get swapped. */
    solve_line(2);
    solve_line(5);
    g_cells[g_row_start[5]] = CELL_EMPTY;
    g_cells[g_row_start[5] + 1] = CELL_EMPTY;
    /* Row 2 is finished, so it's skipped; columns cross both rows. */
    CHECK(pick_swap(&a, &b));
    CHECK(a < b);

    /* Ink lands where the marks are. */
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    g_cells[g_row_start[10] + 10] = CELL_FILLED;
    g_cells[g_row_start[11] + 11] = CELL_MARKED;
    CHECK_EQ(pick_ink(2), 4);
    CHECK(ink_at(g_row_start[10] + 10) && ink_at(g_row_start[11] + 11));
    /* Completing row 10 washes its ink. */
    {
        u8 line = 10;
        LinesResult res;
        battle_complete_lines(&line, 1, g_puzzle.width, &res);
        CHECK_EQ(res.ink_washed, 2);
        CHECK(!ink_at(g_row_start[10] + 10));
        CHECK_EQ(ink_total(), 2);
    }
    CHECK_EQ(battle_use_gadget(EFFECT_WASH, 0), 0);
    CHECK_EQ(ink_total(), 0);

    /* The live line: furthest through, ignoring struck lines. */
    memset(g_cells, CELL_EMPTY, sizeof(g_cells));
    solve_line(LINE_COL + 7);
    {
        u8 live = pick_live_line();
        CHECK(live == LINE_COL + 7);
        g_battle.line[LINE_COL + 7] |= LF_STRUCK;
        CHECK(pick_live_line() != LINE_COL + 7);
    }

    /* A bomb finds unsolved ground. */
    CHECK_EQ(blast_size(15, 15), 5);
    CHECK_EQ(blast_size(10, 10), 3);
    CHECK(pick_blast(5, 0, &top, &left));
    CHECK(top <= 10 && left <= 10);

    /* Shields block hits; gadgets follow chip rules. */
    CHECK_EQ(battle_absorb_hit(), 0);
    battle_use_gadget(EFFECT_SHIELD, 1);
    CHECK_EQ(battle_absorb_hit(), 1);
    CHECK_EQ(battle_absorb_hit(), 0);
    g_battle.enemy_hp = 2;
    CHECK_EQ(battle_use_gadget(EFFECT_BLAST, 2), 1);
    CHECK_EQ(g_battle.enemy_hp, 1);
}

static void test_turns(void)
{
    u8 n = 15;

    start_fight("Weathervane");
    CHECK_EQ(g_enemy.rotate, 6);
    /* Row r becomes column n-1-r; column c becomes row c. */
    CHECK_EQ(turn_line(0, n), LINE_COL + 14);
    CHECK_EQ(turn_line(LINE_COL + 3, n), 3);
    /* Cell (r, c) goes to (c, n-1-r). */
    CHECK_EQ(turn_index(g_row_start[2] + 5, n), g_row_start[5] + 12);
    battle_turn_up(4);
    CHECK(!battle_turn_due(6));
    battle_turn_up(2);
    CHECK(battle_turn_due(6));
    g_battle.line[2] = LF_FOG | LF_STRUCK;
    ink_set(g_row_start[2] + 5, 1);
    g_battle.live = LINE_COL + 3;
    battle_turn(n);
    CHECK_EQ(g_battle.line[LINE_COL + 12], LF_FOG | LF_STRUCK);
    CHECK_EQ(g_battle.line[2], 0);
    CHECK(ink_at(g_row_start[5] + 12));
    CHECK(!ink_at(g_row_start[2] + 5));
    CHECK_EQ(g_battle.live, 3);
    CHECK_EQ(g_battle.turns, 1);
    CHECK_EQ(g_battle.turning, 0);
    /* Four turns come back round. */
    battle_turn(n);
    battle_turn(n);
    battle_turn(n);
    CHECK_EQ(g_battle.line[2], LF_FOG | LF_STRUCK);
    CHECK_EQ(g_battle.turns, 0);
}

int main(void)
{
    story_init();
    test_tables();
    test_gremlin_phases();
    test_armor();
    test_fakes();
    test_charge();
    test_line_satisfied();
    test_pickers();
    test_turns();
    TEST_END("battle");
}
