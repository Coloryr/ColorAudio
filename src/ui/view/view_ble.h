#ifndef __VIEW_BLE_H__
#define __VIEW_BLE_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_ble_create(lv_obj_t *parent, lv_event_cb_t prev,
                        lv_event_cb_t play, lv_event_cb_t next,
                        lv_event_cb_t par);
void lv_ble_connect(const char* text);
void lv_ble_disconnect();
void lv_ble_par_display(bool display);

void lv_ble_set_play();
void lv_ble_set_pause();

void lv_ble_set_title(const char *data);
void lv_ble_set_artlist(const char *data);
void lv_ble_set_album(const char *data);

void lv_ble_set_all_time(float time);
void lv_ble_set_now_time(float time);

void lv_ble_fft_load();
void lv_ble_set_fft_data(uint16_t index, uint16_t value);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_BLE_H__