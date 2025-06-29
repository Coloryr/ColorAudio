#ifndef LV_DEMO_MUSIC_MAIN_H
#define LV_DEMO_MUSIC_MAIN_H

#include "view.h"

lv_obj_t *lv_music_main_create(lv_obj_t *parent);

void lv_music_set_all_time(float time);
void lv_music_set_now_time(float time);
void lv_music_set_title(uint8_t *data);
void lv_music_set_album(uint8_t *data);
void lv_music_set_auther(uint8_t *data);
void lv_music_set_image(uint8_t *data, uint32_t size);
void lv_music_set_fft_data(uint16_t index, uint16_t value, uint32_t size);
void lv_music_fft_load();
void lv_music_fft_clear();
void lv_music_set_sound_info(uint16_t bit, uint32_t rate, uint8_t channel);
void lv_music_set_play_mode();
void lv_music_set_volume(float value);

void lv_music_set_play();
void lv_music_set_pause();

void lv_music_fadein();

#endif