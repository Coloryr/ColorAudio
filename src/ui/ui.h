#ifndef _UI_H_
#define _UI_H_

#include "../music/lyric.h"

#include "lvgl.h"

void view_init();
void view_tick();

void view_set_lyric(lyric_node_t *lyric, lyric_node_t *tlyric);

void view_go_music();

#endif