#include "battle.h"
#include "board.h"

/* A port of the web game's src/game/battle.ts (ADR-010). Each function
 * names the web function it mirrors; keep them in step with it. */

Enemy g_enemy;
BattleState g_battle;

u8 line_has(u8 id, u8 flag)
{
    return (g_battle.line[id] & flag) != 0;
}

u8 battle_count(u8 flag)
{
    u8 i;
    u8 n = 0;

    for (i = 0; i < LINE_COUNT; i++) {
        if (g_battle.line[i] & flag)
            n++;
    }
    return n;
}

/* web armorUp */
u8 battle_armor_up(void)
{
    return battle_count(LF_ARMOR) != 0;
}

static void add_list(const u8 *list, u8 flag)
{
    u8 n;

    if (list == 0)
        return;
    n = *list++;
    while (n) {
        g_battle.line[*list] |= flag;
        list++;
        n--;
    }
}

/* web startBattle */
void battle_start(void)
{
    u8 *p = (u8 *)&g_battle;
    u16 i;

    for (i = 0; i < sizeof(BattleState); i++)
        p[i] = 0;
    g_battle.enemy_hp = g_enemy.hp;
    g_battle.live = LINE_NONE;
    add_list(g_enemy.fog, LF_FOG);
    if (g_enemy.has_armor)
        add_list(g_enemy.weak, LF_ARMOR);
    add_list(g_enemy.scramble, LF_SCRAMBLE);
    add_list(g_enemy.fakes, LF_FAKE);
    add_list(g_enemy.suspects, LF_SUSPECT);
}

/* web currentForm */
void battle_form(Form *f)
{
    u8 i;
    const Phase *p;

    f->name = g_enemy.name;
    f->sprite = g_enemy.sprite;
    f->attack = g_enemy.attack;
    f->charge = g_enemy.charge;
    f->rotate = g_enemy.rotate;
    for (i = 0; i < g_battle.phase; i++) {
        p = &g_enemy.phases[i];
        if (p->name)
            f->name = p->name;
        if (p->sprite != PHASE_KEEP)
            f->sprite = p->sprite;
        if (p->attack != PHASE_KEEP)
            f->attack = p->attack;
        if (p->charge)
            f->charge = p->charge;
        if (p->rotate)
            f->rotate = p->rotate;
    }
    f->armored = battle_armor_up();
    if (g_enemy.has_armor && f->armored)
        f->attack = g_enemy.armor_attack;
}

static u8 turn_line_by(u8 id, u8 n, u8 turns)
{
    while (turns) {
        id = turn_line(id, n);
        turns--;
    }
    return id;
}

/* web addFromPool: up to count lines from the pool, in order, rows first
 * (the web turns the pool into a {rows, cols} set, then picks rows, then
 * columns), skipping lines already in the set and lines that have struck. */
static void add_from_pool(u8 flag, const u8 *pool, u8 count, u8 size)
{
    u8 n;
    u8 i;
    u8 id;
    u8 pass;
    u8 budget = count;
    u8 col;

    if (pool == 0)
        return;
    n = pool[0];
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < n; i++) {
            id = pool[1 + i];
            if (g_battle.turns)
                id = turn_line_by(id, size, g_battle.turns);
            col = LINE_IS_COL(id) ? 1 : 0;
            if (col != pass)
                continue;
            if (budget == 0)
                return;
            if (g_battle.line[id] & (flag | LF_STRUCK))
                continue;
            g_battle.line[id] |= flag;
            budget--;
        }
    }
}

/* web advancePhase */
u8 battle_advance_phase(u8 size)
{
    u8 entered = 0;
    const Phase *p;

    while (g_battle.phase < g_enemy.phase_count) {
        p = &g_enemy.phases[g_battle.phase];
        if (g_battle.enemy_hp > p->at_hp)
            break;
        entered++;
        g_battle.phase++;
        add_from_pool(LF_FOG, p->fog, p->fog_count, size);
        add_from_pool(LF_SCRAMBLE, p->scramble, p->scramble_count, size);
    }
    return entered;
}

/* web liftLines: clears a flag from the lines; returns how many had it. */
static u8 lift(const u8 *lines, u8 n, u8 flag)
{
    u8 i;
    u8 lifted = 0;
    u8 id;

    for (i = 0; i < n; i++) {
        id = lines[i];
        if (g_battle.line[id] & flag) {
            g_battle.line[id] &= (u8)~flag;
            lifted++;
        }
    }
    return lifted;
}

u8 ink_at(u16 cell)
{
    return (g_battle.ink[cell >> 3] >> (cell & 7)) & 1;
}

void ink_set(u16 cell, u8 on)
{
    u8 bit = (u8)(1 << (cell & 7));

    if (on)
        g_battle.ink[cell >> 3] |= bit;
    else
        g_battle.ink[cell >> 3] &= (u8)~bit;
}

u16 ink_total(void)
{
    u16 i;
    u16 n = 0;

    for (i = 0; i < g_puzzle.cells; i++) {
        if (ink_at(i))
            n++;
    }
    return n;
}

/* web washInk / clearInk: ink in the completed lines washes off. */
static u8 wash_lines(const u8 *lines, u8 n, u8 width)
{
    u8 i;
    u8 k;
    u8 id;
    u8 washed = 0;
    u16 cell;

    for (i = 0; i < n; i++) {
        id = lines[i];
        for (k = 0; k < width; k++) {
            if (LINE_IS_COL(id))
                cell = g_row_start[k] + (id - LINE_COL);
            else
                cell = g_row_start[id] + k;
            if (ink_at(cell)) {
                ink_set(cell, 0);
                washed++;
            }
        }
    }
    return washed;
}

/* web chip: chip damage for completed lines that haven't struck. */
static void chip(const u8 *lines, u8 n, LinesResult *out)
{
    u8 i;
    u8 id;
    u8 fresh = 0;
    u8 cracked = 0;
    u8 armored = battle_armor_up();
    u8 damage;
    u8 hp;

    for (i = 0; i < n; i++) {
        id = lines[i];
        if (g_battle.line[id] & LF_STRUCK)
            continue;
        g_battle.line[id] |= LF_STRUCK;
        fresh++;
        if (armored) {
            if (g_battle.line[id] & LF_ARMOR) {
                g_battle.line[id] &= (u8)~LF_ARMOR;
                cracked++;
            }
        }
    }
    out->cracked = cracked;
    out->broke = 0;
    out->dealt = 0;
    if (fresh == 0)
        return;
    damage = fresh * CHIP_DAMAGE;
    if (armored) {
        damage = cracked * WEAK_POINT_DAMAGE;
        out->broke = battle_armor_up() ? 0 : 1;
    }
    hp = g_battle.enemy_hp;
    if (damage >= hp)
        g_battle.enemy_hp = 1;
    else
        g_battle.enemy_hp = hp - damage;
    out->dealt = hp - g_battle.enemy_hp;
}

/* web completeLines (fight/lines.ts), the rules: fog lifts, ink washes off,
 * wipes and honest suspects clear, scrambles restore, chip damage lands,
 * phases start. */
void battle_complete_lines(const u8 *lines, u8 n, u8 width, LinesResult *out)
{
    out->fog_revealed = lift(lines, n, LF_FOG);
    out->ink_washed = wash_lines(lines, n, width);
    out->unwiped = lift(lines, n, LF_WIPED);
    out->honest = lift(lines, n, LF_SUSPECT);
    out->unscrambled = lift(lines, n, LF_SCRAMBLE);
    chip(lines, n, out);
    out->phase_first = g_battle.phase;
    out->phases = battle_advance_phase(width);
}

/* web armorHitsBack */
u8 battle_armor_hits_back(const u8 *lines, u8 n, const u8 *weak_before)
{
    u8 i;
    u8 others = 0;
    u8 total;
    u8 hits = 0;

    if (battle_armor_up() == 0) {
        g_battle.armor_lines = 0;
        return 0;
    }
    for (i = 0; i < n; i++) {
        if ((weak_before[lines[i]] & LF_ARMOR) == 0)
            others++;
    }
    if (others == 0)
        return 0;
    total = g_battle.armor_lines + others;
    while (total >= ARMOR_HIT_LINES) {
        total -= ARMOR_HIT_LINES;
        hits++;
    }
    g_battle.armor_lines = total;
    return hits;
}

/* web warmUpLines: round((w + h) * WARM_UP_SHARE), with the share 1/k. */
u8 battle_warm_up_lines(u8 width, u8 height)
{
    u8 sum = width + height + (WARM_UP_DIV >> 1);
    u8 n = 0;

    while (sum >= WARM_UP_DIV) {
        sum -= WARM_UP_DIV;
        n++;
    }
    return n ? n : 1;
}

/* web chargeUp */
void battle_charge_up(const u8 *lines, u8 n, u8 limit, ChargeResult *out)
{
    u8 i;
    u8 live = g_battle.live;
    u8 charged;

    out->grounded = 0;
    out->zapped = 0;
    if (live == LINE_NONE && g_battle.warm_up) {
        g_battle.warm_up = g_battle.warm_up > n ? g_battle.warm_up - n : 0;
        return;
    }
    if (live == LINE_NONE)
        return;
    for (i = 0; i < n; i++) {
        if (lines[i] == live)
            out->grounded = 1;
    }
    charged = out->grounded ? 0 : g_battle.charged + n;
    out->zapped = charged >= limit ? 1 : 0;
    if (out->grounded || out->zapped) {
        g_battle.charged = 0;
        g_battle.live = LINE_NONE;
        return;
    }
    g_battle.charged = charged;
}

/* web turnUp */
void battle_turn_up(u8 n)
{
    g_battle.turning += n;
}

/* web turnDue */
u8 battle_turn_due(u8 limit)
{
    return limit && g_battle.turning >= limit;
}

/* web turnBattle: every line and cell the battle marks turns with the
 * board; the count to the next turn starts over. */
void battle_turn(u8 n)
{
    static u8 turned[LINE_COUNT];
    static u8 ink[MAX_CELLS / 8];
    u8 i;
    u16 c;
    u16 cells = g_row_start[n - 1] + n;

    for (i = 0; i < LINE_COUNT; i++)
        turned[i] = 0;
    for (i = 0; i < n; i++) {
        turned[turn_line(i, n)] = g_battle.line[i];
        turned[turn_line(LINE_COL + i, n)] = g_battle.line[LINE_COL + i];
    }
    for (i = 0; i < LINE_COUNT; i++)
        g_battle.line[i] = turned[i];
    for (i = 0; i < MAX_CELLS / 8; i++)
        ink[i] = 0;
    for (c = 0; c < cells; c++) {
        if (ink_at(c)) {
            u16 t = turn_index(c, n);
            ink[t >> 3] |= (u8)(1 << (t & 7));
        }
    }
    for (i = 0; i < MAX_CELLS / 8; i++)
        g_battle.ink[i] = ink[i];
    if (g_battle.live != LINE_NONE)
        g_battle.live = turn_line(g_battle.live, n);
    g_battle.turns = (g_battle.turns + 1) & 3;
    g_battle.turning = 0;
}

/* web callOut */
u8 battle_call_out(u8 id, u8 *dealt)
{
    u8 hp = g_battle.enemy_hp;

    *dealt = 0;
    if ((g_battle.line[id] & LF_FAKE) == 0) {
        /* An honest suspect: you paid for the answer, so its mask comes off. */
        g_battle.line[id] &= (u8)~LF_SUSPECT;
        return 0;
    }
    g_battle.enemy_hp = hp > CALL_OUT_DAMAGE ? hp - CALL_OUT_DAMAGE : 1;
    g_battle.line[id] &= (u8)~(LF_FAKE | LF_SUSPECT);
    *dealt = hp - g_battle.enemy_hp;
    return 1;
}

/* web heal (battle.ts): an honest clue accused in a Relaxed run. */
u8 battle_heal(void)
{
    u8 hp = g_battle.enemy_hp;
    u16 healed = (u16)hp + FALSE_ACCUSATION_HEAL;

    if (healed > g_enemy.hp)
        healed = g_enemy.hp;
    g_battle.enemy_hp = (u8)healed;
    return g_battle.enemy_hp - hp;
}

/* web useGadget */
u8 battle_use_gadget(u8 effect, u8 value)
{
    u8 hp = g_battle.enemy_hp;
    u8 i;

    if (effect == EFFECT_SHIELD) {
        g_battle.shield += value;
        return 0;
    }
    if (effect == EFFECT_WASH) {
        for (i = 0; i < MAX_CELLS / 8; i++)
            g_battle.ink[i] = 0;
        return 0;
    }
    g_battle.enemy_hp = hp > value ? hp - value : 1;
    return hp - g_battle.enemy_hp;
}

/* web absorbHit */
u8 battle_absorb_hit(void)
{
    if (g_battle.shield == 0)
        return 0;
    g_battle.shield--;
    return 1;
}

/* web finish */
u8 battle_finish(void)
{
    u8 dealt = g_battle.enemy_hp;

    g_battle.enemy_hp = 0;
    return dealt;
}

/* web bump */
void battle_bump(u8 count)
{
    if (g_battle.counts[count] < 255)
        g_battle.counts[count]++;
}
