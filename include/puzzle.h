#ifndef PUZZLE_H
#define PUZZLE_H

#include "types.h"
#include "pack.h"

#define MAX_SIDE 20
#define MAX_CELLS 400
/* Clues per line: 20 cells hold at most 10 runs. 16 so a line's clues
 * start at line << 4 (816-tcc: a shift, not a multiply call). */
#define CLUE_STRIDE 16

/* The 8-byte-a-slot info table (docs/PUZZLES.md), for walks over the
 * whole pack. */
const u8 *pack_info_table(void);
#define PACK_INFO_WIDTH 4
#define PACK_INFO_COLLECTION 7

/* Read-only facts about a pack slot, straight from ROM. */
u8 pack_width(u16 slot);
u8 pack_height(u16 slot);
u8 pack_passes(u16 slot);
u8 pack_collection(u16 slot);
/* The slot's rows in ROM: MSB first, each row padded to whole bytes. */
const u8 *pack_rows(u16 slot);
/* NUL-terminated upper-case name in ROM. */
const u8 *pack_name(u16 slot);
/* Rank points a solve of this slot is worth (by side, as the web game). */
u8 pack_points(u16 slot);
/* Difficulty band 0..4 (EASY..INSANE) from solver passes. */
u8 pack_band(u16 slot);

/* Collections: display order of slots, count, name. */
u8 collection_size(u8 c);
u16 collection_slot(u8 c, u8 i);
const u8 *collection_name(u8 c);
u16 collection_emblem(u8 c);

/* Murals: big pictures of tiles (15x15 pack puzzles with consecutive
 * slots, row by row, collection MURAL_COLLECTION). */
u8 mural_across(u8 m);
u8 mural_down(u8 m);
u16 mural_first_slot(u8 m);
const u8 *mural_name(u8 m);
/* The mural a tile slot belongs to, and its tile number (0-based). */
u8 mural_of(u16 slot, u8 *tile);
#define MURAL_TILE 15

/* The puzzle in play, decoded into RAM. */
typedef struct {
    u16 slot;
    u8 width;
    u8 height;
    u16 cells;
    /* Filled cells in the solution. */
    u16 filled;
} PuzzleHeader;

extern PuzzleHeader g_puzzle;
/* 1 where the solution is filled; index row_start[r] + c. */
extern u8 g_solution[MAX_CELLS];
extern u16 g_row_start[MAX_SIDE];
/* Clue numbers: row r's are g_row_clues[(r << 4) + k], k < count. A line
 * with no filled cells has one clue, 0. */
extern u8 g_row_clues[MAX_SIDE * CLUE_STRIDE];
extern u8 g_col_clues[MAX_SIDE * CLUE_STRIDE];
extern u8 g_row_clue_count[MAX_SIDE];
extern u8 g_col_clue_count[MAX_SIDE];
/* The most clues any row (column) has. */
extern u8 g_max_row_clues;
extern u8 g_max_col_clues;

void puzzle_load(u16 slot);

/* Works the clues out from g_solution again. */
void puzzle_derive_clues(void);
/* Turns the square puzzle in play a quarter clockwise (a fight, ADR-010). */
void puzzle_turn(void);

#endif
