#ifndef __USB_VIEW_H__
#define __USB_VIEW_H__

#include "lvgl.h"

void view_usb_set_display(bool display);
void view_usb_set_header();
void view_usb_set_fft_data(uint16_t index, uint16_t value);
void view_usb_tick();
void view_usb_update(bool connect);
void view_usb_enable(uint8_t enable);

void view_usb_create(lv_obj_t *parent);

#endif // __USB_VIEW_H__