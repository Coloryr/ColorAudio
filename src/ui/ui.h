#ifndef _UI_H_
#define _UI_H_

#include "../music/lyric.h"

#include "lvgl.h"

typedef enum
{
    LYRIC_NONE = 0,
    LYRIC_FAIL,
    LYRIC_CLEAR,
    LYRIC_UNKNOW = -1
} lyric_state;

void view_init();
void view_tick();
void view_set_lyric_state(lyric_state state);
void view_set_lyric(LyricParser *lyric, LyricParser *tlyric);
void view_set_image_data(uint32_t width, uint32_t height, uint8_t *data);

void view_go_music();

void view_set_check(uint32_t index, bool enable);

#endif