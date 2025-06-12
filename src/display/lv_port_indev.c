#include <stdlib.h>
#include "lvgl.h"

#ifdef BUILD_ARM
static lv_indev_t *indev_touchpad = NULL;
#else
#include "drivers/sdl/lv_sdl_mouse.h"
static lv_indev_t *lvMouse;
static lv_indev_t *lvMouseWheel;
static lv_indev_t *lvKeyboard;
#endif

void lv_port_indev_init(int rot)
{
    lv_disp_t *disp;

    disp = lv_display_get_default();

#ifdef BUILD_ARM
    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event1");
    lv_indev_set_display(touch, disp);
#else
    lvMouse = lv_sdl_mouse_create();
    lvMouseWheel = lv_sdl_mousewheel_create();
    lvKeyboard = lv_sdl_keyboard_create();
#endif
}

