#ifndef _UTILS_H_
#define _UTILS_H_

#include "stdint.h"
#include "lvgl.h"

uint32_t get_length(uint8_t *buffer);
uint32_t utf16_to_utf8(uint16_t *input, uint8_t **output, uint32_t size);

#endif