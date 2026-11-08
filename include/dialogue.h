#ifndef DIALOGUE_H
#define DIALOGUE_H

#include "types.h"

/* The scene runner: a port of the web game's src/game/dialogue.ts
 * (startDialogue, currentStep, advance, choose, speakOnStage). Pure: the
 * scene player (scene_view.c) draws what it says. */

enum { STEP_LINE, STEP_CHOICES, STEP_END };

typedef struct {
    u8 scene;
    u8 node;
    u8 line;    /* past the node's lines: at its choices */
    u8 flags;   /* one bit per flag (story_const.h FLAG_*) */
    u8 ended;
} Dialogue;

/* Up to this many choices show at once. */
#define MAX_CHOICES 4

typedef struct {
    u8 kind;
    u8 who;                   /* the cast member speaking, or 0xFF */
    u16 text;                 /* a line's string id */
    u8 choices;               /* how many choices show */
    u8 choice[MAX_CHOICES];   /* their indexes in the node */
} Step;

void dialogue_start(Dialogue *d, u8 scene, u8 flags);
void dialogue_step(const Dialogue *d, Step *out);
void dialogue_advance(Dialogue *d);
/* index: the choice's index in the node (Step.choice[i]). */
void dialogue_choose(Dialogue *d, u8 index);
u8 dialogue_check(u8 cond, u8 flags);

/* web speakOnStage: who's on stage left to right, and who spoke last. */
#define STAGE_SIZE 3
typedef struct {
    u8 count;
    u8 cast[STAGE_SIZE];
    u8 recent[STAGE_SIZE];    /* most recent first */
} Stage;

void stage_clear(Stage *s);
/* Returns the place (0..2) the speaker stands in. */
u8 stage_speak(Stage *s, u8 who);

#endif
