#ifndef _VIEW_ANIM_H_
#define _VIER_ANIM_H_

#include "lvgl.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void anim_opa_cb(void *var, int32_t v);
void anim_hide_on_done_cb(lv_anim_t *a);

void anim_set_display(lv_obj_t *obj, bool display);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif