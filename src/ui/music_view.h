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

void view_muisc_list_clear();
void view_muisc_list_reload(play_item *item);
void view_muisc_list_add(play_item *item);
void view_music_list_check(uint32_t index, bool state);

void view_music_create(lv_obj_t *parent);
void view_music_tick();
void view_music_update_info();
void view_music_clear_info();
void view_music_update_img();
void view_music_mp4_update();
void view_music_update_state();
void view_music_update_list();
void view_music_init_list();

#endif // __MUSIC_VIEW_H__