#ifndef _VIEW_MUSIC_LIST_H_
#define _VIEW_MUSIC_LIST_H_

#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t index;
    lv_obj_t *view;
    lv_obj_t *title;
    lv_obj_t *auther;
    lv_obj_t *time;
} view_play_item_t;

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_music_list_create(lv_obj_t *parent, lv_event_cb_t clear, lv_event_cb_t search);
view_play_item_t * view_list_add_item(const char *title, const char *artist, uint32_t time, lv_event_cb_t click);

void view_music_list_search_display(bool display);
void view_music_list_search_text(char *data);
void item_check(view_play_item_t *item, bool state);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif