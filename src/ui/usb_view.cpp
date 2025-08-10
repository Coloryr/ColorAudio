#include "usb_view.h"
#include "header_view.h"

#include "view/view_usb.h"

#include "../usb/usb_audio.h"

#include "lvgl.h"

static lv_obj_t *view_obj;

static char rate[32];
static char bits[32];

static void click_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target_obj(event);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        bool chk = lv_obj_get_state(obj) & LV_STATE_CHECKED;
        if (chk)
        {
            usb_audio_set_mode(lv_usb_get_mode());
            lv_usb_get_rate(rate, sizeof(rate));
            usb_audio_set_rate(rate);
            lv_usb_get_bits(bits, sizeof(bits));
            usb_audio_set_bits(bits);
            usb_audio_change(true);
        }
        else
        {
            usb_audio_change(false);
        }
    }
}

void view_usb_set_header()
{
    view_header_move(view_obj);
    view_header_back_display(true);
}

void view_usb_set_display(bool display)
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

void view_usb_create(lv_obj_t *parent)
{
    view_obj = lv_usb_create(parent, click_event);
}