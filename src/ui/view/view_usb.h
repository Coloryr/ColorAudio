#ifndef __VIEW_USB_H__
#define __VIEW_USB_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t lv_usb_get_mode();
uint32_t lv_usb_get_rate();
uint32_t lv_usb_get_bits();

void lv_usb_lock(bool lock);
void lv_usb_fft_load();
void lv_usb_set_fft_data(uint16_t index, uint16_t value);
void lv_usb_set_format(bool connect, uint32_t rate, uint32_t bits);
void lv_usb_set_enable(bool enable);
void lv_usb_set_mode(uint8_t mode);
void lv_usb_set_rate(uint8_t rate);
void lv_usb_set_bits(uint8_t bits);

lv_obj_t *lv_usb_create(lv_obj_t *parent, lv_event_cb_t click, lv_event_cb_t change);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_USB_H__