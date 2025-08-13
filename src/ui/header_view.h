#ifndef __HEADER_VIEW_H__
#define __HEADER_VIEW_H__

#include "lvgl.h"

#include "view/view_header.h"

void view_header_update();
void view_header_wifi(bool off, wifi_rf_state state);
void view_header_headphone1(bool in);
void view_header_headphone2(bool in);
void view_header_move(lv_obj_t *parent);
void view_header_back_display(bool display, bool none);
void view_header_create(lv_obj_t *parent);

#endif // __HEADER_VIEW_H__