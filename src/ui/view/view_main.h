#ifndef _VIEW_MIAN_H_
#define _VIEW_MIAN_H_

#include "lvgl.h"

typedef enum
{
    MAIN_BUTTON_MUSIC = 0,
    MAIN_BUTTON_BLE,
    MAIN_BUTTON_USB,
    MAIN_BUTTON_SETTING,
    MAIN_BUTTON_UNKONW = -1
} main_button_type;

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_main_create(lv_obj_t *parent, lv_event_cb_t cb);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif