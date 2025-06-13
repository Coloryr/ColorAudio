#ifndef _VIEW_H_
#define _VIEW_H_

#include "lvgl.h"

#define LV_DEMO_MUSIC_HANDLE_SIZE 40

#ifdef __cplusplus
extern "C" {
#endif

void view_init();

const char * lv_demo_music_get_title(uint32_t track_id);
const char * lv_demo_music_get_artist(uint32_t track_id);
const char * lv_demo_music_get_genre(uint32_t track_id);
uint32_t lv_demo_music_get_track_length(uint32_t track_id);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif