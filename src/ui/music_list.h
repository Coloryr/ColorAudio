#ifndef _MUSIC_LIST_H_
#define _MUSIC_LIST_H_

#include "lvgl.h"

#include "../music/music.h"

lv_obj_t *lv_music_list_create(lv_obj_t *parent);

void lv_list_clear();
void lv_muisc_list_item_reload(play_item *item);
void lv_list_add_item(play_item *item);
void view_music_list_button_check(uint32_t index, bool state);

#endif
