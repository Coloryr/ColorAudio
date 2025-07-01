#ifndef _VIEW_INPUT_H_
#define _VIEW_INPUT_H_

#include "lvgl.h"

#include <stdbool.h>

typedef void(*input_done_t)(bool iscancel);

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* lv_input_create(lv_obj_t *parent);
void input_close();
void input_show(char *input, uint16_t max_len, input_done_t call);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif