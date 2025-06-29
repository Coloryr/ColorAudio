#ifndef _VIEW_INPUT_H_
#define _VIEW_INPUT_H_

#include "lvgl.h"

#include <stdbool.h>

typedef void(*input_done_t)(bool iscancel);

lv_obj_t* lv_input_create(lv_obj_t *parent);
void input_close();
void input_show(uint8_t *input, uint16_t max_len, input_done_t call);

#endif