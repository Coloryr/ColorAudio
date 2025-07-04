#ifndef _VIEW_TOP_INFO_H_
#define _VIEW_TOP_INFO_H_

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* lv_info_create(lv_obj_t* parent);

void lv_info_scr_display(bool display);
void lv_info_display(bool display);

void lv_info_set_text(const char *text);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif