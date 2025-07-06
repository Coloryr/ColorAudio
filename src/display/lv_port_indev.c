#include <stdlib.h>
#include "lvgl.h"

#ifdef BUILD_ARM
static lv_indev_t *lv_touchpad = NULL;
#else
#include "drivers/sdl/lv_sdl_mouse.h"
static lv_indev_t *lv_mouse;
static lv_indev_t *lv_mouse_wheel;
static lv_indev_t *lv_keyboard;
#endif

void lv_port_indev_init()
{
    lv_disp_t *disp;

    disp = lv_display_get_default();

#ifdef BUILD_ARM
    lv_touchpad = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event1");
    lv_indev_set_display(lv_touchpad, disp);
#else
    lv_mouse = lv_sdl_mouse_create();
    lv_mouse_wheel = lv_sdl_mousewheel_create();
    lv_keyboard = lv_sdl_keyboard_create();
#endif
}

