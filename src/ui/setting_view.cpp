#include "setting_view.h"
#include "header_view.h"

#include "view/view_setting.h"

#include "lvgl.h"

static lv_obj_t *view_obj;

static void wifi_power_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // wifi_power_set(enabled);
}

static void wifi_state_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // wifi_power_set(enabled);
}

static void codec_switch_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // codec_set_enabled(enabled);
}

static void scan_wifi_handler(lv_event_t *e)
{
    // scan_wifi_networks();
}

void view_setting_set_header()
{
    view_header_move(view_obj);
    view_header_back_display(true, true);
}

void view_setting_set_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(view_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(view_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_setting_create(lv_obj_t *parent)
{
    view_obj = lv_setting_create(parent, wifi_power_handler, wifi_state_handler,
                                 scan_wifi_handler, codec_switch_handler);
}