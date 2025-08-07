#ifndef __VIEW_WAVE_H__
#define __VIEW_WAVE_H__

#include "lvgl.h"

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_wave_images_create(lv_obj_t *parent, uint8_t type);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_WAVE_H__