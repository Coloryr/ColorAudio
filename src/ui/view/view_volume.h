#ifndef __VIEW_VOLUME_H__
#define __VIEW_VOLUME_H__

#include "lvgl.h"

#include <stdbool.h>

#define LV_MUSIC_VOLUME_DISPLAY_TIME 8

typedef struct 
{
    lv_obj_t* view;
    lv_obj_t* slider;
    lv_obj_t* mute;
    lv_timer_t* timer;
    bool is_display;
    uint8_t display_down;
} view_volume_t;

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_volume_create(lv_obj_t *parent, lv_event_cb_t volume, lv_event_cb_t mute);
void lv_volume_click(lv_obj_t *view);
void lv_volume_close(lv_obj_t *view);
void lv_volume_set_value(lv_obj_t *view, int32_t value);
void lv_volume_set_dir_hor(lv_obj_t *view, int width);
void lv_volume_show(lv_obj_t *view);
void lv_volume_timer_close(lv_obj_t *view);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_VOLUME_H__