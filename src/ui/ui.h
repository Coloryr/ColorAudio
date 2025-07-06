#ifndef _UI_H_
#define _UI_H_

#include "../music/lyric.h"

#include "lvgl.h"

typedef enum
{
    LYRIC_NONE = 0,
    LYRIC_FAIL,
    LYRIC_UNKNOW = -1
} lyric_state;

void view_init();
void view_tick();
void view_set_lyric_state(lyric_state state);
void view_set_lyric(lyric_node_t *lyric, lyric_node_t *tlyric);

void view_go_music();

void view_set_check(uint32_t index, bool enable);

#endif