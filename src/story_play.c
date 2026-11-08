#include "audio.h"
#include "battle.h"
#include "board.h"
#include "board_view.h"
#include "history.h"
#include "input.h"
#include "play.h"
#include "puzzle.h"
#include "save.h"
#include "scene.h"
#include "story.h"
#include "story_play.h"
#include "story_run.h"
#include "text_wrap.h"
#include "video.h"

/* A story encounter on the play screen (ADR-010; the web's
 * run/encounter.ts, fight/*, dig.ts and rest.ts): the rules come from
 * battle.c; this decides when they run and what the corner says.
 *
 * Order after each frame's board changes, as the web's onLinesComplete:
 * the newly complete lines that count (not still forged) go through
 * battle_complete_lines (fog, ink, wipes, suspects, scrambles, chip,
 * phases), then the armor's hit-back, the turn meter and the charge.
 * Phases' swaps and turns wait for the stroke to end, like the web's. */

u8 g_play_story;

static u8 s_enc;
static u8 s_fight;
static u8 s_kind;
static u8 s_fake[LINE_COUNT][11];
static u8 s_fake_n[LINE_COUNT];
static u8 s_swaps;
static u8 s_inks;
static u8 s_ink_size[MAX_PHASES];
static u8 s_ink_count[MAX_PHASES];
static u8 s_msg[64];
static u8 s_card;          /* a phase card is up for this many frames */
static u8 s_strokes;       /* strokes since the last save */
static u8 s_out;           /* knocked out */
static u8 s_finisher;
static u8 s_reward;
static u8 s_healed;
static Form s_form;

/* --- the corner ------------------------------------------------------ */

static u8 corner_w(void)
{
    return g_view_bx > 2 ? (u8)(g_view_bx - 2) : 1;
}

static void say(const char *text)
{
    u8 i = 0;

    while (text[i] && i < sizeof s_msg - 1) {
        s_msg[i] = (u8)text[i];
        i++;
    }
    s_msg[i] = 0;
}

/* The first corner row free for messages: under the meters, if any. */
static u8 s_msg_y;

static void draw_msg(void)
{
    u8 rows = g_view_by > s_msg_y + 1 ? (u8)(g_view_by - s_msg_y - 1) : 1;

    if (rows > 6)
        rows = 6;
    text_draw_wrapped(1, s_msg_y, s_msg, corner_w(), rows, TEXT_INK);
}

static void draw_hp(u8 y, const char *label, u8 hp, u8 max, u8 pal)
{
    u8 x = bg3_text(1, y, label, TEXT_DIM);

    bg3_fill(x, y, (u8)(corner_w() + 1 - x), 1, 0);
    bg3_number(x, y, hp, 2, 0, pal);
    bg3_text((u8)(x + 2), y, "/", TEXT_DIM);
    bg3_number((u8)(x + 3), y, max, 2, 0, TEXT_INK);
}

static void draw_meters(void)
{
    u8 x = 1;
    u8 n;

    bg3_fill(1, 4, corner_w(), 2, 0);
    s_msg_y = 4;
    if (s_fight == 0)
        return;
    if (s_form.charge || s_form.rotate)
        s_msg_y = 5;
    if (s_form.armored || battle_count(LF_FAKE) || g_battle.shield)
        s_msg_y = 6;
    if (s_form.charge) {
        bg3_put(x, 4, BG_ENTRY(GLYPH_BOLT - 32, TEXT_GOLD) | BG_PRIO);
        if (g_battle.live == LINE_NONE && g_battle.warm_up) {
            bg3_number((u8)(x + 1), 4, g_battle.warm_up, 2, 0, TEXT_DIM);
        } else {
            bg3_number((u8)(x + 1), 4, g_battle.charged, 1, 0, TEXT_INK);
            bg3_text((u8)(x + 2), 4, "/", TEXT_DIM);
            bg3_number((u8)(x + 3), 4, s_form.charge, 1, 0, TEXT_INK);
        }
        x = (u8)(x + 5);
    }
    if (s_form.rotate) {
        bg3_put(x, 4, BG_ENTRY(GLYPH_TURN - 32, TEXT_INK) | BG_PRIO);
        bg3_number((u8)(x + 1), 4, g_battle.turning, 1, 0, TEXT_INK);
        bg3_text((u8)(x + 2), 4, "/", TEXT_DIM);
        bg3_number((u8)(x + 3), 4, s_form.rotate, 1, 0, TEXT_INK);
    }
    x = 1;
    if (s_form.armored) {
        bg3_put(x, 5, BG_ENTRY(GLYPH_TARGET - 32, TEXT_SELECTED) | BG_PRIO);
        bg3_number((u8)(x + 1), 5, battle_count(LF_ARMOR), 1, 0, TEXT_INK);
        x = (u8)(x + 3);
    }
    n = battle_count(LF_FAKE);
    if (n) {
        bg3_put(x, 5, BG_ENTRY(GLYPH_MASK - 32, TEXT_SELECTED) | BG_PRIO);
        bg3_number((u8)(x + 1), 5, n, 1, 0, TEXT_INK);
        x = (u8)(x + 3);
    }
    if (g_battle.shield) {
        bg3_put(x, 5, BG_ENTRY(GLYPH_PATCH - 32, TEXT_OK) | BG_PRIO);
        bg3_number((u8)(x + 1), 5, g_battle.shield, 1, 0, TEXT_INK);
    }
}

static void draw_hud(void)
{
    view_clear_corner(0, g_view_by);
    if (s_fight) {
        battle_form(&s_form);
        bg3_text_n(1, 1, (const char *)s_form.name, corner_w(), TEXT_SELECTED);
        draw_hp(2, "HP ", g_battle.enemy_hp, g_enemy.hp, TEXT_SELECTED);
    } else {
        bg3_text_n(1, 1, (const char *)story_string(enc_name(s_enc)), corner_w(),
            TEXT_SELECTED);
        if (s_kind == ENC_REST) {
            bg3_text(1, 2, "REST", TEXT_DIM);
            bg3_time(6, 2, enc_rest_seconds(s_enc) > g_play.seconds
                ? (u16)(enc_rest_seconds(s_enc) - g_play.seconds) : 0, TEXT_INK);
        } else {
            bg3_text(1, 2, "DIG", TEXT_DIM);
        }
    }
    if (g_run.mode == MODE_HEARTS)
        draw_hp(3, "YOU ", g_run.hp, g_run.max_hp, TEXT_OK);
    else
        bg3_text(1, 3, "RELAXED", TEXT_DIM);
    draw_meters();
    draw_msg();
}

/* --- what the board shows (board_view hooks) ------------------------- */

static u8 shown_clue(u8 id, u8 *out)
{
    const u8 *real;
    u8 n;
    u8 i;

    if (s_fight && (g_battle.line[id] & LF_FAKE)) {
        n = s_fake_n[id];
        for (i = 0; i < n; i++)
            out[i] = s_fake[id][i];
        return n;
    }
    if (LINE_IS_COL(id)) {
        real = g_col_clues + ((u16)(id - LINE_COL) << 4);
        n = g_col_clue_count[id - LINE_COL];
    } else {
        real = g_row_clues + ((u16)id << 4);
        n = g_row_clue_count[id];
    }
    for (i = 0; i < n; i++)
        out[i] = real[i];
    if (n == 1 && out[0] == 0)
        n = 0;
    return n;
}

/* web boardView shown(): a fogged clue hides its numbers; a scrambled one
 * shows them ascending. */
static u8 clue_hook(u8 id, u8 *out)
{
    u8 n = shown_clue(id, out);
    u8 i;
    u8 j;
    u8 t;

    if (n == 0) {
        out[0] = 0;
        n = 1;
    }
    if (g_battle.line[id] & LF_FOG) {
        for (i = 0; i < n; i++)
            out[i] = 0xFE;
    } else if (g_battle.line[id] & LF_SCRAMBLE) {
        for (i = 1; i < n; i++)
            for (j = i; j > 0 && out[j - 1] > out[j]; j--) {
                t = out[j];
                out[j] = out[j - 1];
                out[j - 1] = t;
            }
    }
    return n;
}

static u8 done_hook(u8 id)
{
    static u8 clue[11];
    u8 n = shown_clue(id, clue);

    return line_satisfied(id, clue, n);
}

static u8 ink_hook(u16 index)
{
    return ink_at(index);
}

/* A mark next to a line's clues (BG3): fog, scramble, weak point, suspect,
 * wipe, the live line. Rows: left of the clue band; columns: above it,
 * where there's room. */
static void draw_marker(u8 id)
{
    u8 f = g_battle.line[id];
    u8 glyph = ' ';
    u8 pal = TEXT_SELECTED;
    s8 x;
    s8 y;
    u8 big = g_puzzle.width <= 10 ? 1 : 0;

    if (id == g_battle.live) {
        glyph = GLYPH_BOLT;
        pal = TEXT_GOLD;
    } else if (f & LF_FOG) {
        glyph = '?';
    } else if (f & LF_ARMOR) {
        glyph = GLYPH_TARGET;
    } else if (f & LF_SUSPECT) {
        glyph = GLYPH_MASK;
    } else if (f & LF_WIPED) {
        glyph = GLYPH_LIPS;
    } else if (f & LF_SCRAMBLE) {
        glyph = GLYPH_GEAR;
    }
    if (LINE_IS_COL(id)) {
        x = (s8)(g_view_bx + ((id - LINE_COL) << big));
        y = (s8)(g_view_by - (g_max_col_clues << big) - 1);
    } else {
        x = (s8)(g_view_bx - (g_max_row_clues << big) - 1);
        y = (s8)(g_view_by + (id << big));
    }
    if (x < 0 || y < 0)
        return;
    /* The corner keeps the HUD: no column marks over it. */
    bg3_put((u8)x, (u8)y, BG_ENTRY(glyph - 32, pal) | BG_PRIO);
}

static void draw_markers(void)
{
    u8 i;

    for (i = 0; i < g_puzzle.height; i++)
        draw_marker(i);
    for (i = 0; i < g_puzzle.width; i++)
        draw_marker(LINE_COL + i);
}

static void redraw_lines(void)
{
    u8 i;

    for (i = 0; i < g_puzzle.height; i++)
        view_redraw_line(i);
    for (i = 0; i < g_puzzle.width; i++)
        view_redraw_line(LINE_COL + i);
    draw_markers();
}

/* --- saving --------------------------------------------------------- */

static void save_board(void)
{
    u16 i;
    u8 *cells = g_run.cells;
    u8 v;

    for (i = 0; i < 100; i++)
        cells[i] = 0;
    for (i = 0; i < g_puzzle.cells; i++) {
        v = g_cells[i] == CELL_MISTAKE ? CELL_MARKED : g_cells[i];
        cells[i >> 2] |= (u8)(v << ((i & 3) << 1));
    }
    g_run.board_on = 1;
    g_run.seconds = g_play.seconds;
    g_run.hints_used = g_play.hints_used;
    g_run.mistakes = g_play.mistakes;
    if (s_fight) {
        u8 *src = (u8 *)&g_battle;
        for (i = 0; i < sizeof(BattleState); i++)
            g_run.battle[i] = src[i];
        g_run.battle_on = 1;
    }
}

static void load_board(void)
{
    u16 i;

    for (i = 0; i < g_puzzle.cells; i++)
        g_cells[i] = (u8)((g_run.cells[i >> 2] >> ((i & 3) << 1)) & 3);
    g_play.seconds = g_run.seconds;
    g_play.hints_used = g_run.hints_used;
    g_play.mistakes = g_run.mistakes;
    board_recount();
}

/* --- entering --------------------------------------------------------- */

static void make_fakes(void)
{
    static u8 clue[11];
    u8 id;
    u8 n;
    u8 len;

    for (id = 0; id < LINE_COUNT; id++) {
        s_fake_n[id] = 0;
        if ((g_battle.line[id] & LF_FAKE) == 0)
            continue;
        if (LINE_IS_COL(id)) {
            n = g_col_clue_count[id - LINE_COL];
            len = g_puzzle.height;
        } else {
            n = g_row_clue_count[id];
            len = g_puzzle.width;
        }
        shown_clue(id, clue);  /* the real one: no forgery yet */
        if (n == 1 && clue[0] == 0)
            n = 0;
        n = fake_clue(clue, n, len, s_fake[id]);
        s_fake_n[id] = n == 0xFF ? 0 : n;
    }
}

void story_play_enter(void)
{
    u8 r = run_current();
    u8 i;
    u8 *dst;

    s_enc = run_encounter(r, g_run.node);
    s_kind = enc_kind(s_enc);
    s_fight = s_kind == ENC_FIGHT;
    puzzle_load(enc_puzzle(s_enc));
    play_restart();
    g_play.cur_x = 0;
    g_play.cur_y = 0;
    g_play.mode = (s_fight && g_run.mode == MODE_HEARTS) ? MODE_BATTLE : MODE_RELAXED;
    if (s_fight) {
        story_load_enemy(s_enc);
        if (g_run.battle_on) {
            dst = (u8 *)&g_battle;
            for (i = 0; i < sizeof(BattleState); i++)
                dst[i] = g_run.battle[i];
        } else {
            battle_start();
        }
        /* The saved board is already turned; turn the puzzle to match. */
        for (i = 0; i < g_battle.turns; i++)
            puzzle_turn();
        g_view_clue_hook = clue_hook;
        g_view_ink_hook = ink_hook;
        make_fakes();
    }
    g_view_done_hook = done_hook;
    g_view_hud_hook = draw_hud;
    if (g_run.board_on)
        load_board();
    s_swaps = 0;
    s_inks = 0;
    s_card = 0;
    s_strokes = 0;
    s_out = 0;
    s_msg[0] = 0;
    g_view_new_count = 0;
    if (s_fight)
        say("Lines hurt it. The picture finishes it.");
    else if (s_kind == ENC_REST)
        say("Solve it in time for a full rest.");
    else
        say("Something's buried here.");
}

/* Called after view_draw_all: the marks on the clues. */
void story_play_drawn(void)
{
    if (s_fight)
        draw_markers();
    g_view_new_count = 0;
}

void story_play_leave(void)
{
    g_view_clue_hook = 0;
    g_view_done_hook = 0;
    g_view_ink_hook = 0;
    g_view_hud_hook = 0;
    g_play_story = 0;
}

/* --- the fight -------------------------------------------------------- */

static void take_hit(u8 amount, const char *lead)
{
    if (battle_absorb_hit()) {
        say("The patch kit blocks it.");
        audio_play(SFX_HINT, 10);
        return;
    }
    run_damage(amount);
    audio_play(SFX_MISTAKE, 13);
    say(lead);
    if (g_run.status == RUN_LOST)
        s_out = 1;
}

static void phase_card(u8 first, u8 n)
{
    const Phase *p;
    u8 i;
    u8 y = 11;

    battle_form(&s_form);
    bgSetDisable(0);
    view_cursor(0);
    bg3_fill(0, 8, 32, 12, 0);
    bg3_center(9, (const char *)s_form.name, TEXT_SELECTED);
    for (i = 0; i < n; i++) {
        p = &g_enemy.phases[first + i];
        if (p->taunt)
            y = (u8)(y + text_draw_wrapped(3, y, p->taunt, 26, 3, TEXT_INK));
    }
    audio_play(SFX_NEWS, 12);
    s_card = 90;
}

static void close_card(void)
{
    bg3_fill(0, 8, 32, 12, 0);
    bgSetEnable(0);
    view_cursor(1);
    draw_hud();
    redraw_lines();
}

/* The charge's live line (web aim): set once warm, kept while readable. */
static void aim(void)
{
    if (s_form.charge == 0)
        return;
    if (g_battle.warm_up_set == 0 && g_battle.live == LINE_NONE) {
        g_battle.warm_up = battle_warm_up_lines(g_puzzle.width, g_puzzle.height);
        g_battle.warm_up_set = 1;
    }
    if (g_battle.warm_up)
        return;
    if (g_battle.live != LINE_NONE
        && (g_battle.line[g_battle.live] & (LF_FOG | LF_SCRAMBLE | LF_SUSPECT | LF_STRUCK)) == 0)
        return;
    g_battle.live = pick_live_line();
}

static void lines_done(void)
{
    static u8 counted[VIEW_NEW_MAX];
    static u8 fresh[VIEW_NEW_MAX];
    static u8 weak[LINE_COUNT];
    LinesResult res;
    ChargeResult cr;
    u8 n = 0;
    u8 nf = 0;
    u8 i;
    u8 id;
    u8 hits;

    for (i = 0; i < g_view_new_count; i++) {
        id = g_view_new_lines[i];
        if (g_battle.line[id] & LF_FAKE)
            continue;  /* a forgery doesn't count until it's called out */
        counted[n++] = id;
        if ((g_battle.line[id] & LF_STRUCK) == 0)
            fresh[nf++] = id;
    }
    g_view_new_count = 0;
    if (n == 0)
        return;
    for (i = 0; i < LINE_COUNT; i++)
        weak[i] = g_battle.line[i];
    battle_complete_lines(counted, n, g_puzzle.width, &res);
    if (res.dealt) {
        audio_play(SFX_LINE, 11);
        say(res.cracked ? "Weak point! Triple damage." : "A hit!");
    } else if (battle_armor_up() && nf) {
        say("The armor shrugs it off.");
    }
    if (res.broke)
        say("The armor breaks!");
    if (res.fog_revealed)
        say("You solved through the fog.");
    if (res.unscrambled)
        say("Gears click back into place.");
    if (res.honest)
        say("That one checks out: honest.");
    if (res.ink_washed)
        say("The ink washes away.");
    hits = 0;
    if (g_enemy.has_armor)
        hits = battle_armor_hits_back(fresh, nf, weak);
    battle_form(&s_form);
    if (s_form.rotate && nf)
        battle_turn_up(nf);
    if (s_form.charge && nf) {
        battle_charge_up(fresh, nf, s_form.charge, &cr);
        if (cr.grounded)
            say("Grounded! The charge drains.");
        if (cr.zapped) {
            if (g_run.mode == MODE_HEARTS) {
                battle_bump(COUNT_ZAPS);
                take_hit(ZAP_DAMAGE, "Zap!");
            } else {
                say("A zap fizzles (Relaxed).");
            }
        }
    }
    while (hits--) {
        if (g_run.mode == MODE_HEARTS)
            take_hit(ARMOR_HIT_DAMAGE, "The armor hits back!");
        else
            say("The armor hits back. It fizzles.");
    }
    if (res.phases) {
        for (i = 0; i < res.phases; i++) {
            const Phase *p = &g_enemy.phases[res.phase_first + i];
            if (p->swap)
                s_swaps++;
            if (p->ink && s_inks < MAX_PHASES) {
                s_ink_size[s_inks] = p->ink;
                s_ink_count[s_inks] = p->ink_count;
                s_inks++;
            }
        }
        phase_card(res.phase_first, res.phases);
    }
    aim();
    redraw_lines();
    draw_hud();
}

/* web swaps.ts run(): the Mimic swaps and wipes two lines. */
static void do_swap(void)
{
    u8 a;
    u8 b;
    u8 k;
    u8 n;
    u16 at;
    u8 j;

    if (pick_swap(&a, &b) == 0)
        return;
    for (j = 0; j < 2; j++) {
        u8 id = j ? b : a;
        n = LINE_IS_COL(id) ? g_puzzle.height : g_puzzle.width;
        for (k = 0; k < n; k++) {
            at = LINE_IS_COL(id) ? g_row_start[k] + (id - LINE_COL) : g_row_start[id] + k;
            board_set(at, CELL_EMPTY);
        }
        g_battle.line[id] |= LF_WIPED;
    }
    history_reset();
    say("It swapped two lines and ate your marks!");
    audio_play(SFX_NEWS, 12);
}

/* web turns.ts turn(): the board, its clues and the battle turn. */
static void do_turn(void)
{
    static u8 cells[MAX_CELLS];
    u8 n = g_puzzle.width;
    u16 i;

    for (i = 0; i < g_puzzle.cells; i++)
        cells[turn_index(i, n)] = g_cells[i];
    for (i = 0; i < g_puzzle.cells; i++)
        g_cells[i] = cells[i];
    battle_turn(n);
    puzzle_turn();
    board_recount();
    make_fakes();
    history_reset();
    view_draw_all();
    draw_markers();
    say("The board turns a quarter clockwise.");
    audio_play(SFX_NEWS, 12);
}

/* After a stroke: swaps, ink and turns that were waiting. */
static void after_stroke(void)
{
    u8 i;
    u16 inked = 0;

    while (s_swaps) {
        s_swaps--;
        do_swap();
    }
    for (i = 0; i < s_inks; i++) {
        u8 k;
        for (k = 0; k < s_ink_count[i]; k++)
            inked += pick_ink(s_ink_size[i]);
    }
    s_inks = 0;
    if (inked) {
        say("Ink! Cells go dark.");
        view_refresh_all();
    }
    battle_form(&s_form);
    if (battle_turn_due(s_form.rotate))
        do_turn();
    if (++s_strokes >= 8) {
        s_strokes = 0;
        save_board();
        save_write();
    }
}

u8 story_play_frame(u8 stroke_ended)
{
    if (s_card) {
        if (--s_card == 0 || (g_pressed & (KEY_A | KEY_B)))  {
            s_card = 0;
            close_card();
        }
        return STORY_BUSY;
    }
    if (s_fight) {
        while (g_play.hits) {
            g_play.hits--;
            battle_bump(COUNT_WRONG_FILLS);
            battle_form(&s_form);
            take_hit(s_form.attack, "Wrong cell! It hits you.");
            draw_hud();
        }
        if (g_view_new_count)
            lines_done();
        if (stroke_ended && s_out == 0)
            after_stroke();
        if (s_out) {
            save_board();
            save_write();
            return STORY_KNOCKED_OUT;
        }
    } else {
        g_view_new_count = 0;
        if (s_kind == ENC_REST && (g_play.frames == 0))
            draw_hud();
    }
    return STORY_PLAYING;
}

/* --- gadgets and call-outs -------------------------------------------- */

u8 story_play_call_out(u8 col)
{
    u8 id = col ? (u8)(LINE_COL + g_play.cur_x) : g_play.cur_y;
    u8 dealt;

    if (s_fight == 0 || battle_count(LF_SUSPECT) == 0)
        return 0;
    if ((g_battle.line[id] & LF_SUSPECT) == 0) {
        say("That clue isn't a suspect.");
        draw_hud();
        return 1;
    }
    if (battle_call_out(id, &dealt)) {
        say("A forgery! Unmasked.");
        audio_play(SFX_LINE, 12);
        /* Its real clue: if the line already fits it, it completes now. */
        if (done_hook(id) && g_view_new_count < VIEW_NEW_MAX)
            g_view_new_lines[g_view_new_count++] = id;
        view_refresh_all();
    } else if (g_run.mode == MODE_HEARTS) {
        battle_bump(COUNT_WRONG_ACCUSATIONS);
        battle_form(&s_form);
        take_hit(s_form.attack, "Honest! It hits you for that.");
    } else {
        battle_heal();
        say("Honest. It patches itself up.");
    }
    redraw_lines();
    draw_hud();
    return 1;
}

u8 story_play_gadget(u8 i)
{
    u8 g = g_run.gadgets[i] - 1;
    u8 effect = gadget_effect(g);
    u8 top;
    u8 left;
    u8 size;
    u8 r;
    u8 c;
    u16 at;

    if (s_fight == 0)
        return 0;
    if (effect == EFFECT_SHIELD && g_run.mode != MODE_HEARTS) {
        say("Enemies can't hurt you in Relaxed.");
        return 0;
    }
    if (effect == EFFECT_WASH && ink_total() == 0) {
        say("There's no ink to wash.");
        return 0;
    }
    battle_bump(COUNT_GADGETS_USED);
    if (effect == EFFECT_BLAST) {
        battle_bump(COUNT_BOMBS_USED);
        size = blast_size(g_puzzle.width, g_puzzle.height);
        if (pick_blast(size, (u16)(snes_vblank_count * 25173 + 13849), &top, &left)) {
            for (r = top; r < top + size; r++)
                for (c = left; c < left + size; c++) {
                    at = g_row_start[r] + c;
                    board_set(at, g_solution[at] ? CELL_FILLED : CELL_MARKED);
                    ink_set(at, 0);
                }
            history_reset();
            if (board_solved())
                g_play.solved = 1;
        }
        say("Boom! Part of the board is revealed.");
    } else if (effect == EFFECT_WASH) {
        battle_bump(COUNT_SPONGES_USED);
        say("The sponge wipes the ink away.");
        view_refresh_all();
    } else {
        say("The patch kit will block the next hit.");
    }
    battle_use_gadget(effect, gadget_value(g));
    run_gadget_remove(i);
    {
        u8 n = battle_advance_phase(g_puzzle.width);
        if (n)
            phase_card((u8)(g_battle.phase - n), n);
    }
    redraw_lines();
    draw_hud();
    return 1;
}

/* --- endings ---------------------------------------------------------- */

/* The encounter is solved (web encounter.ts complete). */
void story_play_solved(void)
{
    u8 gadget = enc_gadget(s_enc);
    u8 before;
    u8 heal;

    s_finisher = s_fight ? battle_finish() : 0;
    if (s_fight)
        run_add_tally(g_battle.counts, g_play.hints_used);
    else
        run_add_tally((const u8 *)"\0\0\0\0\0\0", g_play.hints_used);
    run_complete_encounter();
    s_reward = STORY_NONE;
    if (gadget != STORY_NONE && run_gadget_add(gadget))
        s_reward = gadget;
    before = g_run.hp;
    if (s_kind == ENC_REST) {
        heal = g_play.seconds <= enc_rest_seconds(s_enc) ? enc_rest_heal(s_enc) : REST_LATE_HEAL;
        run_heal(heal);
    }
    s_healed = g_run.hp - before;
    save_write();
}

/* The corner after a solve: what it did, then A to go on. */
void story_play_draw_result(void)
{
    view_clear_corner(1, g_view_by);
    bg3_text(1, 1, s_fight ? "DEFEATED!" : "CLEARED!", TEXT_SELECTED);
    if (s_fight) {
        bg3_text(1, 2, "FINISHER", TEXT_DIM);
        bg3_number(1, 3, s_finisher, 3, 0, TEXT_INK);
    }
    if (s_reward != STORY_NONE) {
        bg3_text(1, 4, "GOT", TEXT_DIM);
        bg3_text_n(1, 5, (const char *)story_string(gadget_name(s_reward)), corner_w(), TEXT_OK);
    }
    if (s_healed) {
        bg3_text(1, 6, "+", TEXT_OK);
        bg3_number(2, 6, s_healed, 2, 0, TEXT_OK);
        bg3_text(4, 6, " HP", TEXT_OK);
    }
    bg3_text(1, (u8)(g_view_by > 2 ? g_view_by - 1 : 1), "A GO ON", TEXT_DIM);
}

void story_play_quit(void)
{
    if (g_run.status == RUN_ACTIVE)
        save_board();
    save_write();
}

/* Where A goes after a solve or a knock-out. */
u8 story_play_next_scene(void)
{
    return g_run.status == RUN_ACTIVE ? SCENE_MAP : SCENE_RUN_RESULT;
}
