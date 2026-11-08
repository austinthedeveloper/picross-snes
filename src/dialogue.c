#include "dialogue.h"
#include "story.h"

/* A port of src/game/dialogue.ts (ADR-010). */

u8 dialogue_check(u8 cond, u8 flags)
{
    u8 on;

    if (cond == 0)
        return 1;
    on = (flags >> ((cond & 0x7F) - 1)) & 1;
    return (cond & 0x80) ? !on : on;
}

static u8 visible_choices(const u8 *node, u8 flags, u8 *out)
{
    u8 n = node_choice_count(node);
    u8 i;
    u8 shown = 0;

    for (i = 0; i < n; i++) {
        if (dialogue_check(CHOICE_COND(node_choice(node, i)), flags)) {
            if (out && shown < MAX_CHOICES)
                out[shown] = i;
            shown++;
        }
    }
    return shown;
}

/* web settle: moves to the next thing that should show. */
static void settle(Dialogue *d)
{
    const u8 *scene = story_scene(d->scene);
    const u8 *node;
    u8 hops;

    for (hops = 0; hops < 250; hops++) {
        node = scene_node(scene, d->node);
        while (d->line < NODE_LINES(node)
               && !dialogue_check(LINE_COND(node_line(node, d->line)), d->flags))
            d->line++;
        if (d->line < NODE_LINES(node))
            return;
        if (visible_choices(node, d->flags, 0))
            return;
        if (NODE_NEXT(node) == STORY_NONE) {
            d->ended = 1;
            return;
        }
        d->node = NODE_NEXT(node);
        d->line = 0;
    }
    d->ended = 1;
}

void dialogue_start(Dialogue *d, u8 scene, u8 flags)
{
    d->scene = scene;
    d->node = scene_start(story_scene(scene));
    d->line = 0;
    d->flags = flags;
    d->ended = 0;
    settle(d);
}

void dialogue_step(const Dialogue *d, Step *out)
{
    const u8 *node;
    const u8 *line;

    out->choices = 0;
    out->who = STORY_NONE;
    if (d->ended) {
        out->kind = STEP_END;
        return;
    }
    node = scene_node(story_scene(d->scene), d->node);
    if (d->line < NODE_LINES(node)) {
        line = node_line(node, d->line);
        out->kind = STEP_LINE;
        out->who = LINE_WHO(line);
        out->text = LINE_TEXT(line);
        return;
    }
    out->kind = STEP_CHOICES;
    out->choices = visible_choices(node, d->flags, out->choice);
    if (out->choices > MAX_CHOICES)
        out->choices = MAX_CHOICES;
}

void dialogue_advance(Dialogue *d)
{
    Step s;

    dialogue_step(d, &s);
    if (s.kind != STEP_LINE)
        return;
    d->line++;
    settle(d);
}

void dialogue_choose(Dialogue *d, u8 index)
{
    Step s;
    u8 i;
    u8 ok = 0;
    const u8 *choice;

    dialogue_step(d, &s);
    if (s.kind != STEP_CHOICES)
        return;
    for (i = 0; i < s.choices; i++) {
        if (s.choice[i] == index)
            ok = 1;
    }
    if (ok == 0)
        return;
    choice = node_choice(scene_node(story_scene(d->scene), d->node), index);
    d->flags = (u8)((d->flags & ~CHOICE_SET_MASK(choice))
        | (CHOICE_SET_VALUES(choice) & CHOICE_SET_MASK(choice)));
    if (CHOICE_NEXT(choice) == STORY_NONE) {
        d->ended = 1;
        return;
    }
    d->node = CHOICE_NEXT(choice);
    d->line = 0;
    settle(d);
}

void stage_clear(Stage *s)
{
    s->count = 0;
}

/* web speakOnStage: newcomers walk on at the right; when the stage is full
 * they replace whoever has been quiet longest. */
u8 stage_speak(Stage *s, u8 who)
{
    u8 i;
    u8 j;
    u8 at = STAGE_SIZE;
    u8 leaving;

    for (i = 0; i < s->count; i++) {
        if (s->cast[i] == who)
            at = i;
    }
    if (at < STAGE_SIZE) {
        /* Move to the front of recent. */
        for (j = 0; j < s->count && s->recent[j] != who; j++)
            ;
        for (; j > 0; j--)
            s->recent[j] = s->recent[j - 1];
        s->recent[0] = who;
        return at;
    }
    if (s->count < STAGE_SIZE) {
        s->cast[s->count] = who;
        for (j = s->count; j > 0; j--)
            s->recent[j] = s->recent[j - 1];
        s->recent[0] = who;
        s->count++;
        return s->count - 1;
    }
    leaving = s->recent[STAGE_SIZE - 1];
    for (i = 0; i < STAGE_SIZE; i++) {
        if (s->cast[i] == leaving)
            at = i;
    }
    s->cast[at] = who;
    for (j = STAGE_SIZE - 1; j > 0; j--)
        s->recent[j] = s->recent[j - 1];
    s->recent[0] = who;
    return at;
}
