#ifndef _VIEW_H_
#define _VIEW_H_

#include <stdbool.h>
#include <stdint.h>

#define LV_DEMO_MUSIC_HANDLE_SIZE 40

extern bool update_info;
extern bool update_img;
extern bool update_state;
extern bool update_list;

extern bool init_list;

extern bool clear_info;

extern bool update_top_info;

extern bool mp4_have_update;

extern int32_t volume_down;

#ifdef __cplusplus
extern "C" {
#endif

void view_update_info();
void view_update_img();
void view_update_state();
void view_update_list();

void view_init_list();

void view_music_clear();

void view_top_info();

void view_mp4_update();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif