/**
 * @file lv_linux_drm.h
 *
 */

#ifndef LV_LINUX_DRM_H
#define LV_LINUX_DRM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <lvgl.h>

#if LV_USE_LINUX_DRM

#if LV_DRM_USE_RGA
#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_display_t * lv_drm_disp_create(int rot);
int lv_drm_disp_delete(lv_display_t * disp);

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_LINUX_DRM */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_LINUX_DRM_H */
