#include "main_view.h"

#include "view/view_main.h"

#include "../main.h"
#include "../music/music.h"

#include "lvgl.h"

static lv_obj_t *main_view;

static void button_event_cb(lv_event_t *e)
{
    main_button_type *arg = static_cast<main_button_type *>(lv_event_get_user_data(e));
    switch (*arg)
    {
    case MAIN_BUTTON_MUSIC:
        change_mode(MAIN_MODE_MUSIC);
        break;
    case MAIN_BUTTON_BLE:

        break;
    case MAIN_BUTTON_USB:

        break;
    case MAIN_BUTTON_SETTING:

        break;
    default:
        break;
    }
}

void view_main_set_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(main_view, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(main_view, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_main_create(lv_obj_t *parent)
{
    main_view = lv_main_create(parent, button_event_cb);
}