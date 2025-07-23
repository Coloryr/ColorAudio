#ifndef __INPUT_VIEW_H__
#define __INPUT_VIEW_H__

#include "lvgl.h"

typedef void(*input_done_cb)(bool iscancel);

void view_input_close();
void view_input_show(char *input, uint16_t max_len, input_done_cb call);

void view_input_create(lv_obj_t *parent);

#endif // __INPUT_VIEW_H__