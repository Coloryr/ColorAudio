#ifndef __VIEW_HEADER_H__
#define __VIEW_HEADER_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_header_back_display(bool display);

lv_obj_t *lv_header_create(lv_obj_t *parent, lv_event_cb_t back);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_HEADER_H__