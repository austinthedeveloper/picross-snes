/* Host tests for the scene runner (src/dialogue.c), following the web
 * game's src/game/dialogue.test.ts, on the real imported scenes. */
#include <string.h>

#include "dialogue.h"
#include "story.h"
#include "test_support.h"

static const char *text(u16 id)
{
    return (const char *)story_string(id);
}

static void test_first_job_intro(void)
{
    Dialogue d;
    Step s;
    u8 lines = 0;
    const u8 *scene = story_scene(SCENE_ID_FIRST_JOB_INTRO);

    dialogue_start(&d, SCENE_ID_FIRST_JOB_INTRO, 0);
    dialogue_step(&d, &s);
    CHECK_EQ(s.kind, STEP_LINE);
    CHECK_EQ(s.who, STORY_NONE);  /* narration */
    CHECK(strncmp(text(s.text), "Monday at the workshop.", 23) == 0);
    for (;;) {
        dialogue_step(&d, &s);
        if (s.kind != STEP_LINE)
            break;
        lines++;
        dialogue_advance(&d);
    }
    CHECK_EQ(lines, 8);
    CHECK_EQ(s.kind, STEP_CHOICES);
    CHECK_EQ(s.choices, 2);
    CHECK(strcmp(text(CHOICE_TEXT(node_choice(scene_node(scene, d.node), s.choice[0]))),
                 "A normal bat?") == 0);
    /* Advancing does nothing at choices. */
    dialogue_advance(&d);
    dialogue_step(&d, &s);
    CHECK_EQ(s.kind, STEP_CHOICES);
    /* "A normal bat?" goes through "bat", then "go", then ends. */
    dialogue_choose(&d, s.choice[0]);
    dialogue_step(&d, &s);
    CHECK(strcmp(text(s.text), "No bat is normal. This one breathes fog.") == 0);
    CHECK(strcmp(text(scene_cast_name(scene, s.who)), "Dee") == 0);
    lines = 0;
    while (s.kind == STEP_LINE) {
        lines++;
        dialogue_advance(&d);
        dialogue_step(&d, &s);
    }
    CHECK_EQ(lines, 5);
    CHECK_EQ(s.kind, STEP_END);
    CHECK(d.ended);
}

static void test_outro_flags(void)
{
    Dialogue d;
    Step s;

    dialogue_start(&d, SCENE_ID_FIRST_JOB_OUTRO, 1 << FLAG_WON);
    dialogue_step(&d, &s);
    CHECK(strncmp(text(s.text), "Generator's humming", 19) == 0);
    dialogue_advance(&d);
    dialogue_step(&d, &s);
    CHECK_EQ(s.kind, STEP_END);

    dialogue_start(&d, SCENE_ID_FIRST_JOB_OUTRO, 0);
    dialogue_step(&d, &s);
    CHECK(strncmp(text(s.text), "Okay, we didn't get paid.", 25) == 0);
    CHECK(dialogue_check(0, 0));
    CHECK(dialogue_check(1, 1));
    CHECK(!dialogue_check(1 | 0x80, 1));
}

static void test_stage(void)
{
    Stage st;

    stage_clear(&st);
    CHECK_EQ(stage_speak(&st, 5), 0);
    CHECK_EQ(stage_speak(&st, 7), 1);
    CHECK_EQ(stage_speak(&st, 5), 0);
    CHECK_EQ(stage_speak(&st, 9), 2);
    /* Full: the newcomer takes the place of whoever's been quiet longest (7). */
    CHECK_EQ(stage_speak(&st, 4), 1);
    CHECK(st.cast[0] == 5 && st.cast[1] == 4 && st.cast[2] == 9);
    CHECK_EQ(st.recent[0], 4);
    /* Then 5 is quietest. */
    CHECK_EQ(stage_speak(&st, 3), 0);
}

static void test_every_scene_ends(void)
{
    Dialogue d;
    Step s;
    u8 sc;
    u16 guard;

    for (sc = 0; sc < STORY_SCENES; sc++) {
        dialogue_start(&d, sc, 1 << FLAG_WON);
        for (guard = 0; guard < 500; guard++) {
            dialogue_step(&d, &s);
            if (s.kind == STEP_END)
                break;
            if (s.kind == STEP_CHOICES)
                dialogue_choose(&d, s.choice[s.choices - 1]);
            else
                dialogue_advance(&d);
        }
        CHECK_EQ(s.kind, STEP_END);
    }
}

int main(void)
{
    story_init();
    test_first_job_intro();
    test_outro_flags();
    test_stage();
    test_every_scene_ends();
    TEST_END("story");
}
