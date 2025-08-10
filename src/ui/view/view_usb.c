#include "view_usb.h"
#include "view_wave.h"

#include "../lang.h"
#include "../font.h"

#include "lvgl.h"

#include <stdint.h>

static lv_obj_t *mode_obj;
static lv_obj_t *rate_obj;
static lv_obj_t *bits_obj;

const static char *mode = "uac1\n"
                          "uac2";

const static char *rate_uac1 = "44100\n"
                               "48000\n"
                               "96000\n"
                               "44100,48000\n"
                               "44100,48000,96000";

const static char *rate_uac2 = "44100\n"
                               "48000\n"
                               "96000\n"
                               "192000\n"
                               "44100,48000\n"
                               "44100,48000,96000\n"
                               "44100,48000,96000,192000";

const static char *bits_uac1 = "16\n"
                               "24\n"
                               "32";

const static char *bits_uac2 = "16\n"
                               "24\n"
                               "32\n"
                               "16,24\n"
                               "16,24,32";

static void change_choise(uint32_t index)
{
    if (index == 0)
    {
        lv_dropdown_set_options_static(rate_obj, rate_uac1);
        lv_dropdown_set_options_static(bits_obj, bits_uac1);
    }
    else
    {
        lv_dropdown_set_options_static(rate_obj, rate_uac2);
        lv_dropdown_set_options_static(bits_obj, bits_uac2);
    }
}

static void mode_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        change_choise(lv_dropdown_get_selected(obj));
    }
}

uint32_t lv_usb_get_mode()
{
    return lv_dropdown_get_selected(mode_obj);
}

void lv_usb_get_rate(char *buffer, uint32_t size)
{
    lv_dropdown_get_selected_str(rate_obj, buffer, size);
}

void lv_usb_get_bits(char *buffer, uint32_t size)
{
    lv_dropdown_get_selected_str(bits_obj, buffer, size);
}

void lv_usb_lock(bool lock)
{
    if (lock)
    {
        lv_obj_add_state(mode_obj, LV_STATE_DISABLED);
        lv_obj_add_state(rate_obj, LV_STATE_DISABLED);
        lv_obj_add_state(bits_obj, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_remove_state(mode_obj, LV_STATE_DISABLED);
        lv_obj_remove_state(rate_obj, LV_STATE_DISABLED);
        lv_obj_remove_state(bits_obj, LV_STATE_DISABLED);
    }
}

lv_obj_t *lv_usb_create(lv_obj_t *parent, lv_event_cb_t click)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);

    lv_wave_images_create(obj, 0);

    lv_obj_t *lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text1);
    lv_obj_set_style_text_font(lable, font_22, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 60);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text3);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 120);

    mode_obj = lv_dropdown_create(obj);
    lv_obj_set_width(mode_obj, 240);
    lv_dropdown_set_options_static(mode_obj, mode);
    lv_obj_align(mode_obj, LV_ALIGN_DEFAULT, 160, 110);
    lv_obj_add_event_cb(mode_obj, mode_handler, LV_EVENT_ALL, NULL);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text4);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 180);

    rate_obj = lv_dropdown_create(obj);
    lv_obj_set_width(rate_obj, 240);
    lv_obj_align(rate_obj, LV_ALIGN_DEFAULT, 160, 170);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text4);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 240);

    bits_obj = lv_dropdown_create(obj);
    lv_obj_set_width(bits_obj, 240);
    lv_obj_align(bits_obj, LV_ALIGN_DEFAULT, 160, 230);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text2);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 300);

    lv_obj_t *sw = lv_switch_create(obj);
    lv_obj_add_event_cb(sw, click, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_align(sw, LV_ALIGN_DEFAULT, 160, 295);

    lv_dropdown_set_selected(mode_obj, 1);
    change_choise(1);

    return obj;
}