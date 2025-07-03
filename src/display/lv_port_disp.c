#include <stdlib.h>
#include "lvgl.h"
#include "drivers/display/drm/lv_linux_drm.h"
#include "drivers/sdl/lv_sdl_window.h"

static lv_display_t * disp;

void lv_port_disp_init(lv_coord_t hor_res, lv_coord_t ver_res, int rot)
{

#ifdef BUILD_ARM
    lv_display_rotation_t lvgl_rot = LV_DISPLAY_ROTATION_0;

    switch (rot)
    {
    case 0:
        lvgl_rot = LV_DISPLAY_ROTATION_0;
        break;
    case 90:
        lvgl_rot = LV_DISPLAY_ROTATION_90;
        break;
    case 180:
        lvgl_rot = LV_DISPLAY_ROTATION_180;
        break;
    case 270:
        lvgl_rot = LV_DISPLAY_ROTATION_270;
        break;
    default:
        LV_LOG_ERROR("Unsupported rotation %d", rot);
        break;
    }

    LV_LOG_USER("LV_USE_LINUX_DRM");
    disp = lv_drm_disp_create(rot);
#else
    LV_LOG_USER("LV_USE_SDL");
    disp = lv_sdl_window_create(hor_res, ver_res);
#endif
}

