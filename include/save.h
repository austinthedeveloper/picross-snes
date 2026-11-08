#ifndef SAVE_H
#define SAVE_H

#include "types.h"
#include "pack.h"

/* Battery save (ADR-003). Two copies in SRAM, each with a magic, version,
 * sequence and checksum; a write goes to the older copy, so a torn write
 * loses only that write. Records are indexed by pack slot, which never
 * changes (tools/import_pack.py). */
#define SAVE_MAGIC 0x5850
#define SAVE_VERSION 1
#define SAVE_SLOTS 600
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
} SaveData;

#define SAVE_BYTES 2576

extern SaveData g_save;

/* Boot: loads the newest valid copy, or starts a blank save. Returns 1
 * when a save was found. */
u8 save_load(void);
/* Writes g_save to the older copy. Only at safe points (a solve, leaving
 * a menu): the checksum costs a few frames. */
void save_write(void);
void save_blank(void);
/* The checksum of an image as save_write stores it. */
u16 save_checksum(const SaveData *data);

/* SRAM access: src/sram.c on the SNES, an array in the host tests. */
void sram_write_bytes(const u8 *src, u16 offset, u16 len);
void sram_read_bytes(u8 *dest, u16 offset, u16 len);

#endif
