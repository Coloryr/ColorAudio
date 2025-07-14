#ifndef __MUSIC_VIEW_H__
#define __MUSIC_VIEW_H__

#include "../music/lyric.h"
#include "../music/music.h"

#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    LYRIC_NONE = 0,
    LYRIC_FAIL,
    LYRIC_CLEAR,
    LYRIC_UNKNOW = -1
} lyric_state;

void view_music_set_image_data(uint32_t width, uint32_t height, uint8_t *data);
void view_music_set_lyric_state(lyric_state state);
void view_music_set_check(uint32_t index, bool enable);
void view_music_set_lyric(LyricParser *lyric, LyricParser *tlyric);
void view_music_set_display(bool display);

lv_obj_t *lv_music_view_create(lv_obj_t *parent);
void lv_music_view_tick();

void lv_list_clear();
void lv_muisc_list_item_reload(play_item *item);
void lv_list_add_item(play_item *item);
void view_music_list_button_check(uint32_t index, bool state);

#endif // __MUSIC_VIEW_H__