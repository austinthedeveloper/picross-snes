#ifndef SAVE_H
#define SAVE_H

#include "types.h"
#include "pack.h"

/* Battery save (ADR-003). Two copies in SRAM, each with a magic, version,
 * sequence and checksum; a write goes to the older copy, so a torn write
 * loses only that write. Records are indexed by pack slot, which never
 * changes (tools/import_pack.py). */
#define SAVE_MAGIC 0x5850
#define SAVE_VERSION 2
#define SAVE_SLOTS 700
#define SAVE_COPY_B 0x1000
#define SAVE_SRAM_BYTES 0x2000

#if PACK_SAVE_SLOTS != SAVE_SLOTS
#error "tools/gen_assets.py SAVE_SLOTS must equal include/save.h SAVE_SLOTS"
#endif

enum {
    REC_SOLVED = 1,
    REC_HEARTS = 2,   /* solved in hearts mode */
    REC_PERFECT = 4,  /* hearts mode, no mistakes, no hints */
    REC_NOHINT = 8    /* solved without a hint (either mode) */
};

typedef struct {
    /* Best time in seconds; meaningful once REC_SOLVED is set. */
    u16 best;
    u8 flags;
    /* Times solved, stopping at 255. */
    u8 solves;
} Record;

/* The one board kept between sessions: quitting a puzzle part-way. */
typedef struct {
    /* Slot + 1; 0 when nothing is suspended. */
    u16 slot1;
    u16 seconds;
    u8 mode;
    u8 hearts;
    u8 hints_used;
    u8 mistakes;
    u8 cur_x;
    u8 cur_y;
    u8 reserved[2];
    /* Two bits a cell (CELL_*), four cells a byte, first cell lowest. */
    u8 cells[100];
} Suspend;

/* Story progress (ADR-010), version 2 on. Every field's offset is the same
 * on the SNES and the host (u16s at even offsets). */
#define STORY_JOB_CAP 48
#define STORY_SCENE_CAP 128
#define RUN_GADGET_CAP 8

typedef struct {
    u8 plays;
    u8 wins;
    u8 hits_on_wins;  /* Hits-on (hearts mode) wins */
    u8 best_hp;       /* HP left on the best Hits-on win */
} JobStats;

enum { RUN_NONE = 0, RUN_ACTIVE = 1, RUN_WON = 2, RUN_LOST = 3 };

/* The run in progress (web RunState), with its encounter's board and
 * fight. */
typedef struct {
    u8 slot1;         /* job save slot + 1; 0 when there's no run */
    u8 node;          /* the next encounter */
    u8 hp;
    u8 max_hp;
    u8 status;        /* RUN_* */
    u8 mode;          /* MODE_* at the start */
    u8 recorded;      /* its result is in the job stats */
    u8 battle_on;     /* battle holds the current fight */
    u8 gadgets[RUN_GADGET_CAP];  /* gadget index + 1, 0 empty */
    u8 tally[7];      /* web Tally: the fight counts, then hints used */
    u8 has_tally;
    u16 endless_seed;
    u8 endless[6];    /* Endless's pools (M5) */
    u8 board_on;      /* the encounter's board below is in play */
    u8 hints_used;
    u16 seconds;
    u8 hearts;
    u8 mistakes;
    u8 cells[100];    /* two bits a cell, as Suspend */
    u8 battle[106];   /* a BattleState (battle.h) */
} RunSave;

typedef struct {
    JobStats jobs[STORY_JOB_CAP];
    u16 endless_best;
    u8 seen_scenes[STORY_SCENE_CAP / 8];
    u8 seen_tips;     /* one bit per power */
    u8 reserved[29];
    RunSave run;
} StorySave;

/* Version 1's whole layout is version 2's first SAVE_BYTES_V1 bytes. */
#define SAVE_BYTES_V1 2976

typedef struct {
    u16 magic;
    u8 version;
    u8 seq;
    u16 sum;
    u16 reserved_header;
    /* Options. */
    u8 mode;
    u8 sound;
    u8 reserved_options[6];
    /* Totals over every solve. */
    u16 seconds_lo;
    u16 seconds_hi;
    u16 solves;
    u16 hearts_solves;
    u16 perfect_solves;
    u16 nohint_solves;
    /* Room to grow without a version bump: 0 means the default. */
    u8 reserved[36];
    Suspend suspend;
    Record records[SAVE_SLOTS];
    /* Version 2. */
    StorySave story;
} SaveData;

#define SAVE_BYTES 3460

extern SaveData g_save;

/* Boot: loads the newest valid copy, or starts a blank save. Returns 1
 * when a save was found. */
u8 save_load(void);
/* Writes g_save to the older copy. Only at safe points (a solve, leaving
 * a menu): the checksum costs a few frames. */
void save_write(void);
void save_blank(void);
/* The checksum of an image as save_write stores it: its version's bytes. */
u16 save_checksum(const SaveData *data);

/* SRAM access: src/sram.c on the SNES, an array in the host tests. */
void sram_write_bytes(const u8 *src, u16 offset, u16 len);
void sram_read_bytes(u8 *dest, u16 offset, u16 len);

#endif
