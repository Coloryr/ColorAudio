#ifndef __INFO_H__
#define __INFO_H__

#include "lvgl.h"

#include <string>

bool view_top_info_is_display();

void view_top_info_display(std::string info);
void view_top_error_display(std::string info);
void view_top_info_close();
void view_top_info_update();

void view_top_info_create(lv_obj_t *parent);

#endif // __INFO_H__