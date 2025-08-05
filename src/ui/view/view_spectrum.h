#ifndef __VIEW_SPECTRUM_H__
#define __VIEW_SPECTRUM_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_spectrum_create(lv_obj_t *parent);
void lv_spectrum_set_value(lv_obj_t *spectrum, uint16_t index, uint16_t value);
void lv_spectrum_clear_value(lv_obj_t *spectrum);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_SPECTRUM_H__