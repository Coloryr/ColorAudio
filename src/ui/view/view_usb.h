#ifndef __VIEW_USB_H__
#define __VIEW_USB_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t lv_usb_get_mode();
void lv_usb_lock(bool lock);
void lv_usb_get_rate(char *buffer, uint32_t size);
void lv_usb_get_bits(char *buffer, uint32_t size);

lv_obj_t * lv_usb_create(lv_obj_t *parent, lv_event_cb_t click);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_USB_H__