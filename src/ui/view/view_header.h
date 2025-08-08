#ifndef __VIEW_HEADER_H__
#define __VIEW_HEADER_H__

#include "lvgl.h"

typedef enum
{
    WIFI_RF_FULL,
    WIFI_RF_MID,
    WIFI_RF_LOW,
    WIFI_RF_NONE
} wifi_rf_state;

#ifdef __cplusplus
extern "C" {
#endif

void lv_header_back_display(bool display);
void lv_header_wifi_display(bool display);
void lv_header_wifi_set_state(wifi_rf_state state);
void lv_header_headphone_display(uint8_t index, bool display);

lv_obj_t *lv_header_create(lv_obj_t *parent, lv_event_cb_t back);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_HEADER_H__