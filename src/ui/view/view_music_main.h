#ifndef _VIEW_MUSIC_MAIN_H_
#define _VIEW_MUSIC_MAIN_H_

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *view_music_main_create(lv_obj_t *parent, lv_event_cb_t time,
                                 lv_event_cb_t volume, lv_event_cb_t mode,
                                 lv_event_cb_t prev, lv_event_cb_t play, lv_event_cb_t next);

void lv_music_set_all_time(float time);
void lv_music_set_now_time(float time);
void lv_music_set_title(const char *data);
void lv_music_set_album(const char *data);
void lv_music_set_auther(const char *data);
void lv_music_set_image(uint8_t *data, uint32_t size);
void lv_music_set_fft_data(uint16_t index, uint16_t value, uint32_t size);
void lv_music_fft_load();
void lv_music_fft_clear();
void lv_music_set_sound_info(uint16_t bit, uint32_t rate, uint8_t channel, uint32_t bps);
void lv_music_set_play_mode();
void lv_music_set_volume(float value);
void lv_music_volume_close();
void lv_music_img_load();
void lv_music_set_image_data(uint32_t width, uint32_t height, uint8_t *data);

void lv_music_set_play();
void lv_music_set_pause();

void lv_music_fadein();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif