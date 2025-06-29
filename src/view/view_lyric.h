#ifndef _VIEW_LYRIC_H_
#define _VIEW_LYRIC_H_

#include "lvgl.h"

#include <stdint.h>

lv_obj_t *lv_lyric_create(lv_obj_t *parent);
void lv_lyric_set_text(uint8_t* text);
void lv_lyric_tr_set_text(uint8_t* text);

#endif