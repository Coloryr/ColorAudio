#ifndef __BLE_VIEW_H__
#define __BLE_VIEW_H__

#include "lvgl.h"

void view_ble_set_header();
void view_ble_set_display(bool display);
void view_ble_create(lv_obj_t *parent);

void view_ble_tick();
void view_ble_update_info();
void view_ble_update_time();

void view_ble_set_par(uint32_t key);
void view_ble_set_par_close();

void view_ble_par_disable();
void view_ble_par_enable();

void view_ble_set_fft_data(uint16_t index, uint16_t value);

#endif // __BLE_VIEW_H__