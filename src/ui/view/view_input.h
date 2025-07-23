#ifndef _VIEW_INPUT_H_
#define _VIEW_INPUT_H_

#include "lvgl.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void lv_input_set_max_size(uint16_t size);
const char *lv_input_get_text();
void lv_input_set_text(const char* text);
void lv_input_create(lv_obj_t *parent, void(*close_cb)());
void lv_input_display(bool display);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif