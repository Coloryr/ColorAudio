#ifndef _VIEW_H_
#define _VIEW_H_

#include "lvgl.h"

#define LV_DEMO_MUSIC_HANDLE_SIZE 40

#ifdef __cplusplus
extern "C" {
#endif

void view_update_info();
void view_update_img();
void view_update_state();
void view_set_all_time(float time);
void view_set_now_time(float time);
void view_init();
void load_font();

extern const lv_font_t *font_22;
extern const lv_font_t *font_32;

const char * lv_demo_music_get_title(uint32_t track_id);
const char * lv_demo_music_get_artist(uint32_t track_id);
const char * lv_demo_music_get_genre(uint32_t track_id);
uint32_t lv_demo_music_get_track_length(uint32_t track_id);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif