#ifndef _UTILS_H_
#define _UTILS_H_

#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

uint32_t get_length(uint8_t *buffer);
uint32_t utf16_to_utf8(uint16_t *input, uint8_t **output, uint32_t size);
bool load_image(uint8_t *data, uint32_t size, lv_image_dsc_t* img_dsc);

#endif