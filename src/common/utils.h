#ifndef _UTILS_H_
#define _UTILS_H_

#include "lvgl.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 计算一个数据长度，到0结束计算
 * @param buffer 数据
 * @return 数据长度
 */
uint32_t get_length(uint8_t *buffer);
/**
 * 将utf16字符转utf8字符
 * @param input 输入的字符
 * @param output 输出的字符
 * @param size 输入字符的长度
 * @return 输出字符的长度
 */
uint32_t utf16_to_utf8(uint16_t *input, char **output, uint32_t size);
/**
 * 加载图片
 * @param data 图片数据
 * @param size 图片数据大小
 * @param img_dsc 输出的图片
 * @return 是否成功加载
 */
bool load_image(uint8_t *data, uint32_t size, lv_image_dsc_t* img_dsc);
/**
 * 取最小值
 * @param a
 * @param b
 * @return 最小值
 */
uint32_t min(uint32_t a, uint32_t b);

uint32_t utf8_strlen(const char *str);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif