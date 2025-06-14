#ifndef LV_DEMO_MUSIC_MAIN_H
#define LV_DEMO_MUSIC_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "view.h"

lv_obj_t * lv_demo_music_main_create(lv_obj_t * parent);
void lv_demo_music_play(uint32_t id);
void lv_music_resume(void);
void lv_music_pause(void);
void lv_music_album_next(bool next);

void view_set_all_time(float time);
void view_set_now_time(float time);
void view_set_title(uint8_t *data);
void view_set_album(uint8_t *data);
void view_set_auther(uint8_t *data);
void view_set_image(uint8_t* data, uint32_t size);
void view_set_fft_data(uint16_t index, uint16_t value, uint32_t size);
void view_fft_load();

void lv_music_set_resume();
void lv_music_set_pause();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif