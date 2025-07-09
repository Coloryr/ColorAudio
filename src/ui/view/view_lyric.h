#ifndef _VIEW_LYRIC_H_
#define _VIEW_LYRIC_H_

#include "lvgl.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_lyric_create(lv_obj_t *parent);
void lv_lyric_set_text(const char* text);
void lv_lyric_k_set_text(const char *text);
void lv_lyric_k_now_set_text(const char *text);
void lv_lyric_kp_set_text(float kp);
void lv_lyric_tr_set_text(const char* text);
void lv_lyric_set_have_k(bool have);

void lv_lyric_draw();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif