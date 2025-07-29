#ifndef __VIEW_DIALOG_H__
#define __VIEW_DIALOG_H__

#include "lvgl.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void lv_create_dialog(lv_obj_t *parent, lv_event_cb_t cb);
void lv_dialog_set_text(const char *message);

void lv_dialog_display(bool dispaly);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_DIALOG_H__
