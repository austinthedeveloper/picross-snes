#include "input.h"

u16 g_held;
u16 g_pressed;
u16 g_repeat;
static u16 s_prev;
static u16 s_swallowed;
static u8 s_timer;

void input_update(void)
{
    u16 now = pad_keys[0];

    s_swallowed &= now;
    now &= (u16)~s_swallowed;
    g_pressed = now & (u16)~s_prev;
    g_held = now;
    g_repeat = g_pressed;
    if (now & PAD_DIRS) {
        if (g_pressed & PAD_DIRS) {
            s_timer = INPUT_REPEAT_DELAY;
        } else if (--s_timer == 0) {
            s_timer = INPUT_REPEAT_RATE;
            g_repeat |= now & PAD_DIRS;
        }
    }
    s_prev = now;
}

void input_swallow(void)
{
    s_swallowed = pad_keys[0];
    s_prev = 0;
    g_pressed = 0;
    g_repeat = 0;
}
