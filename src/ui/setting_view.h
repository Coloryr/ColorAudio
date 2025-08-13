#ifndef __SETTING_VIEW_H__
#define __SETTING_VIEW_H__

#include "lvgl.h"

void view_setting_set_header();
void view_setting_set_display(bool display);
void view_setting_create(lv_obj_t *parent);

#endif // __SETTING_VIEW_H__