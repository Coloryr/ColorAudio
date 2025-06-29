#ifndef _FONT_H_
#define _FONT_H_

#include "lvgl.h"

#ifdef BUILD_ARM
#define TTF_FONT "/root/MiSans-Regular.ttf"
#else
#define TTF_FONT "/home/coloryr/MiSans-Regular.ttf"
#endif

extern const lv_font_t *font_18;
extern const lv_font_t *font_22;
extern const lv_font_t *font_32;

void load_font();

#endif