#ifndef __SETTING_VIEW_H__
#define __SETTING_VIEW_H__

#include <vector>

#include "lvgl.h"

#include "wireless/wifi.h"

typedef struct
{
    char ssid[256];
    char pwd[256];
} wifi_connect_t;

void view_setting_set_header();
void view_setting_set_display(bool display);
void view_setting_create(lv_obj_t *parent);
void view_setting_wifi_list(std::vector<wifi_item_t> &list);

#endif // __SETTING_VIEW_H__