#include "battle.h"
#include "board.h"

/* The engine helpers fights use, ported from the web game's src/engine:
 * clues.ts (satisfiedClues), fakes.ts, swap.ts, ink.ts, charge.ts,
 * blast.ts and transform.ts (ADR-010). All deterministic except the
 * bomb's spot, which the caller seeds. */

static u8 line_length(u8 id)
{
    return LINE_IS_COL(id) ? g_puzzle.height : g_puzzle.width;
}

/* The board index of the k-th cell of a line. */
static u16 line_cell(u8 id, u8 k)
{
    if (LINE_IS_COL(id))
        return g_row_start[k] + (id - LINE_COL);
    return g_row_start[id] + k;
}

/* A wrong fill becomes an X: the web's Marked. */
static u8 cell_state(u16 i)
{
    u8 c = g_cells[i];

    return c == CELL_MISTAKE ? CELL_MARKED : c;
}

static u8 s_line[MAX_SIDE];
static u8 s_run_start[MAX_SIDE];
static u8 s_run_len[MAX_SIDE];

static u8 load_line(u8 id)
{
    u8 n = line_length(id);
    u8 k;
    u16 at;

    for (k = 0; k < n; k++) {
        at = line_cell(id, k);
        s_line[k] = cell_state(at);
    }
    return n;
}

static u8 closed(s16 i, u8 n)
{
    if (i < 0)
        return 1;
    if (i >= n)
        return 1;
    return s_line[i] == CELL_MARKED;
}

static u8 empty_in(u8 from, u8 to)
{
    u8 k;

    for (k = from; k < to; k++) {
        if (s_line[k] == CELL_EMPTY)
            return 1;
    }
    return 0;
}

/* web satisfiedClues(line, clues).every(Boolean), or for an empty clue
 * !line.includes(Filled) (boardView's dim). */
u8 line_satisfied(u8 id, const u8 *clue, u8 count)
{
    u8 n = load_line(id);
    u8 runs = 0;
    u8 k = 0;
    u8 ci;
    u8 ri;
    u8 left = 0;
    u8 right = 0;
    u8 end;
    u8 all;

    while (k < n) {
        if (s_line[k] != CELL_FILLED) {
            k++;
            continue;
        }
        s_run_start[runs] = k;
        while (k < n && s_line[k] == CELL_FILLED)
            k++;
        s_run_len[runs] = k - s_run_start[runs];
        runs++;
    }
    if (count == 0)
        return runs == 0;
    if (runs == count) {
        all = 1;
        for (ri = 0; ri < runs; ri++) {
            if (s_run_len[ri] != clue[ri])
                all = 0;
        }
        if (all)
            return 1;
    }
    /* From the left: runs locked in from the edge. */
    ci = 0;
    for (ri = 0; ri < runs; ri++) {
        if (ci >= count)
            break;
        if (empty_in(0, s_run_start[ri]))
            break;
        if (closed((s16)s_run_start[ri] + s_run_len[ri], n) == 0)
            break;
        if (s_run_len[ri] != clue[ci])
            break;
        ci++;
    }
    left = ci;
    /* From the right. */
    ci = count;
    ri = runs;
    while (ri > 0 && ci > 0) {
        ri--;
        end = s_run_start[ri] + s_run_len[ri];
        if (empty_in(end, n))
            break;
        if (closed((s16)s_run_start[ri] - 1, n) == 0)
            break;
        if (s_run_len[ri] != clue[ci - 1])
            break;
        ci--;
        right++;
    }
    /* Every clue is done when the two passes cover them all between them. */
    return left + right >= count;
}

static u8 fits(const u8 *clue, u8 count, u8 length)
{
    u8 i;
    u16 sum = 0;

    for (i = 0; i < count; i++)
        sum += clue[i];
    if (count)
        sum += count - 1;
    return sum <= length;
}

static u8 differs(const u8 *a, u8 na, const u8 *b, u8 nb)
{
    u8 i;

    if (na != nb)
        return 1;
    for (i = 0; i < na; i++) {
        if (a[i] != b[i])
            return 1;
    }
    return 0;
}

/* Writes clue with run i replaced by r1 (and r2 when two) into out. */
static u8 replace_run(const u8 *clue, u8 count, u8 i, u8 r1, u8 r2, u8 two, u8 *out)
{
    u8 n = 0;
    u8 k;

    for (k = 0; k < count; k++) {
        if (k == i) {
            out[n++] = r1;
            if (two)
                out[n++] = r2;
        } else {
            out[n++] = clue[k];
        }
    }
    return n;
}

static u8 try_fake(const u8 *clue, u8 count, u8 length, u8 *cand, u8 n, u8 *out)
{
    u8 k;

    if (fits(cand, n, length) == 0)
        return 0;
    if (differs(cand, n, clue, count) == 0)
        return 0;
    for (k = 0; k < n; k++)
        out[k] = cand[k];
    return 1;
}

/* web fakeClue: merge two neighbours, split a long run, lengthen a run,
 * shorten a run; the first that fits and differs. */
u8 fake_clue(const u8 *clue, u8 count, u8 length, u8 *out)
{
    static u8 cand[MAX_SIDE];
    u8 i;
    u8 k;
    u8 n;

    for (i = 0; i + 1 < count; i++) {
        n = 0;
        for (k = 0; k < i; k++)
            cand[n++] = clue[k];
        cand[n++] = clue[i] + clue[i + 1] + 1;
        for (k = i + 2; k < count; k++)
            cand[n++] = clue[k];
        if (try_fake(clue, count, length, cand, n, out))
            return n;
    }
    for (i = 0; i < count; i++) {
        if (clue[i] < 3)
            continue;
        n = replace_run(clue, count, i, clue[i] - 2, 1, 1, cand);
        if (try_fake(clue, count, length, cand, n, out))
            return n;
    }
    for (i = 0; i < count; i++) {
        n = replace_run(clue, count, i, clue[i] + 1, 0, 0, cand);
        if (try_fake(clue, count, length, cand, n, out))
            return n;
    }
    for (i = 0; i < count; i++) {
        if (clue[i] < 2)
            continue;
        n = replace_run(clue, count, i, clue[i] - 1, 0, 0, cand);
        if (try_fake(clue, count, length, cand, n, out))
            return n;
    }
    if (count == 0) {
        cand[0] = 1;
        if (try_fake(clue, count, length, cand, 1, out))
            return 1;
    }
    if (count == 1 && clue[0] == 1)
        return 0;
    return 0xFF;
}

/* web swap.ts lineDone: the line's filled cells match the solution. */
static u8 line_matches(u8 id)
{
    u8 n = line_length(id);
    u8 k;
    u16 at;
    u8 filled;

    for (k = 0; k < n; k++) {
        at = line_cell(id, k);
        filled = cell_state(at) == CELL_FILLED ? 1 : 0;
        if (filled != g_solution[at])
            return 0;
    }
    return 1;
}

/* Correct marks in an unfinished line (web swap.ts isCorrect). */
static u8 swap_damage(u8 id)
{
    u8 n;
    u8 k;
    u16 at;
    u8 c;
    u8 damage = 0;

    if (line_matches(id))
        return 0;
    n = line_length(id);
    for (k = 0; k < n; k++) {
        at = line_cell(id, k);
        c = cell_state(at);
        if (c == CELL_FILLED && g_solution[at])
            damage++;
        else if (c == CELL_MARKED && g_solution[at] == 0)
            damage++;
    }
    return damage;
}

/* web pickSwap: the two unfinished rows, or two unfinished columns,
 * holding the most correct marks; rows win a tie. */
u8 pick_swap(u8 *a, u8 *b)
{
    u8 kind;
    u8 i;
    u8 count;
    u8 base;
    u8 d;
    u8 best1;
    u8 best2;
    u8 d1;
    u8 d2;
    u16 best = 0;
    u8 found = 0;

    for (kind = 0; kind < 2; kind++) {
        base = kind ? LINE_COL : 0;
        count = kind ? g_puzzle.width : g_puzzle.height;
        best1 = LINE_NONE;
        best2 = LINE_NONE;
        d1 = 0;
        d2 = 0;
        for (i = 0; i < count; i++) {
            d = swap_damage(base + i);
            if (d == 0)
                continue;
            if (d > d1) {
                best2 = best1;
                d2 = d1;
                best1 = base + i;
                d1 = d;
            } else if (d > d2) {
                best2 = base + i;
                d2 = d;
            }
        }
        if (best2 == LINE_NONE)
            continue;
        if (found == 0 || (u16)d1 + d2 > best) {
            best = (u16)d1 + d2;
            found = 1;
            if (best1 < best2) {
                *a = best1;
                *b = best2;
            } else {
                *a = best2;
                *b = best1;
            }
        }
    }
    return found;
}

/* web pickInk: the size x size block hiding the most marks, skipping
 * inked cells and struck lines; ties to more cells, then top-left. */
u16 pick_ink(u8 size)
{
    u8 w = g_puzzle.width;
    u8 h = g_puzzle.height;
    u8 s = size;
    u8 top;
    u8 left;
    u8 r;
    u8 c;
    u16 i;
    u8 cells;
    u8 marks;
    u8 best_cells = 0;
    u8 best_marks = 0;
    u8 best_top = 0;
    u8 best_left = 0;
    u8 found = 0;
    u16 added = 0;

    if (s > w)
        s = w;
    if (s > h)
        s = h;
    for (top = 0; top + s <= h; top++) {
        for (left = 0; left + s <= w; left++) {
            cells = 0;
            marks = 0;
            for (r = top; r < top + s; r++) {
                if (g_battle.line[r] & LF_STRUCK)
                    continue;
                for (c = left; c < left + s; c++) {
                    i = g_row_start[r] + c;
                    if (g_battle.line[LINE_COL + c] & LF_STRUCK)
                        continue;
                    if (ink_at(i))
                        continue;
                    cells++;
                    if (g_cells[i] != CELL_EMPTY)
                        marks++;
                }
            }
            if (cells == 0)
                continue;
            if (found == 0 || marks > best_marks
                || (marks == best_marks && cells > best_cells)) {
                found = 1;
                best_marks = marks;
                best_cells = cells;
                best_top = top;
                best_left = left;
            }
        }
    }
    if (found == 0)
        return 0;
    for (r = best_top; r < best_top + s; r++) {
        if (g_battle.line[r] & LF_STRUCK)
            continue;
        for (c = best_left; c < best_left + s; c++) {
            i = g_row_start[r] + c;
            if (g_battle.line[LINE_COL + c] & LF_STRUCK)
                continue;
            if (ink_at(i))
                continue;
            ink_set(i, 1);
            added++;
        }
    }
    return added;
}

/* web pickLiveLine: the unfinished line you're furthest through, then the
 * one with most cells to fill; rows before columns. */
u8 pick_live_line(void)
{
    u8 id;
    u8 n;
    u8 k;
    u16 at;
    u8 size;
    u8 filled;
    u8 best = LINE_NONE;
    u8 best_filled = 0;
    u8 best_size = 0;
    u16 lhs;
    u16 rhs;
    u8 kind;
    u8 count;

    for (kind = 0; kind < 2; kind++) {
        count = kind ? g_puzzle.width : g_puzzle.height;
        for (k = 0; k < count; k++) {
            id = kind ? LINE_COL + k : k;
            if (g_battle.line[id] & (LF_STRUCK | LF_FOG | LF_SCRAMBLE | LF_SUSPECT))
                continue;
            n = line_length(id);
            size = 0;
            filled = 0;
            for (at = 0; at < n; at++) {
                u16 cell = line_cell(id, (u8)at);
                if (g_solution[cell]) {
                    size++;
                    if (cell_state(cell) == CELL_FILLED)
                        filled++;
                }
            }
            if (size == 0)
                continue;
            if (best == LINE_NONE) {
                best = id;
                best_filled = filled;
                best_size = size;
                continue;
            }
            /* filled / size > best_filled / best_size, exactly. */
            lhs = (u16)filled * best_size;
            rhs = (u16)best_filled * size;
            if (lhs > rhs || (lhs == rhs && size > best_size)) {
                best = id;
                best_filled = filled;
                best_size = size;
            }
        }
    }
    return best;
}

/* web blastSize */
u8 blast_size(u8 width, u8 height)
{
    u8 m = width < height ? width : height;

    return m >= 15 ? 5 : 3;
}

/* web revealedCell: what a hint would put there. */
static u8 unsolved(u16 i)
{
    u8 want = g_solution[i] ? CELL_FILLED : CELL_MARKED;

    return cell_state(i) != want;
}

/* web pickBlast: a spot where at least half the blast is unsolved (or, if
 * none, one with the most); rand picks among them. */
u8 pick_blast(u8 size, u16 rand, u8 *top_out, u8 *left_out)
{
    u8 w = g_puzzle.width;
    u8 h = g_puzzle.height;
    u8 s = size;
    u8 top;
    u8 left;
    u8 r;
    u8 c;
    u8 n;
    u8 most = 0;
    u16 threshold2;
    u16 open = 0;
    u16 pick;
    static u8 counts[MAX_SIDE * MAX_SIDE];
    u16 spot;

    if (s > w)
        s = w;
    if (s > h)
        s = h;
    spot = 0;
    for (top = 0; top + s <= h; top++) {
        for (left = 0; left + s <= w; left++) {
            n = 0;
            for (r = top; r < top + s; r++) {
                for (c = left; c < left + s; c++) {
                    if (unsolved(g_row_start[r] + c))
                        n++;
                }
            }
            counts[spot++] = n;
            if (n > most)
                most = n;
        }
    }
    if (most == 0)
        return 0;
    /* unsolved >= min(s*s/2, most), doubled to stay in whole numbers. */
    threshold2 = (u16)s * s;
    if ((u16)most * 2 < threshold2)
        threshold2 = (u16)most * 2;
    for (spot = 0; spot < (u16)(h - s + 1) * (w - s + 1); spot++) {
        if ((u16)counts[spot] * 2 >= threshold2)
            open++;
    }
    pick = (u16)(((u32)rand * open) >> 16);
    spot = 0;
    for (top = 0; top + s <= h; top++) {
        for (left = 0; left + s <= w; left++) {
            if ((u16)counts[spot] * 2 >= threshold2) {
                if (pick == 0) {
                    *top_out = top;
                    *left_out = left;
                    return 1;
                }
                pick--;
            }
            spot++;
        }
    }
    return 0;
}

/* web turnLine: row r becomes column n-1-r; column c becomes row c. */
u8 turn_line(u8 id, u8 n)
{
    if (LINE_IS_COL(id))
        return id - LINE_COL;
    return LINE_COL + (n - 1 - id);
}

/* web turnIndex: cell (r, c) goes to (c, n-1-r). */
u16 turn_index(u16 i, u8 n)
{
    u8 r = 0;
    u16 c = i;

    while (c >= n) {
        c -= n;
        r++;
    }
    return g_row_start[c] + (n - 1 - r);
}
