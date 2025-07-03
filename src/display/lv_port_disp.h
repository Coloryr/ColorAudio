#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化显示部分
 * @param hor_res 宽度
 * @param ver_res 高度
 * @param rot 旋转
 */
void lv_port_disp_init(lv_coord_t hor_res, lv_coord_t ver_res, int rot);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_H*/

