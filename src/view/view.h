#ifndef _VIEW_H_
#define _VIEW_H_

#include "lyric.h"

#include "lvgl.h"

#define LV_DEMO_MUSIC_HANDLE_SIZE 40

void view_update_info();
void view_update_img();
void view_update_state();
void view_init_list();
void view_update_list();
void view_update_list_index();
void lv_music_set_all_time(float time);
void lv_music_set_now_time(float time);
void view_init();
void view_tick();

void view_set_lyric(lyric_node_t *lyric, lyric_node_t *tlyric);

void view_go_music();

#endif