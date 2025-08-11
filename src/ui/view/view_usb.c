#include "view_usb.h"
#include "view_wave.h"
#include "view_spectrum.h"

#include "../lang.h"
#include "../font.h"

#include "lvgl.h"

#include <stdint.h>

static lv_obj_t *mode_obj;
static lv_obj_t *rate_obj;
static lv_obj_t *bits_obj;
static lv_obj_t *spectrum_obj;
static lv_obj_t *format_obj;
static lv_obj_t *sw_obj;

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

uint32_t lv_usb_get_rate()
{
    return lv_dropdown_get_selected(rate_obj);
}

uint32_t lv_usb_get_bits()
{
    return lv_dropdown_get_selected(bits_obj);
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

void lv_usb_set_fft_data(uint16_t index, uint16_t value)
{
    lv_spectrum_set_value(spectrum_obj, index, value);
}

void lv_usb_fft_load()
{
    lv_obj_invalidate(spectrum_obj);
}

void lv_usb_set_format(bool connect, uint32_t rate, uint32_t bits)
{
    if (connect)
    {
        lv_label_set_text_fmt(format_obj, now_lang->usb_text7, rate, bits);
    }
    else
    {
        lv_label_set_text(format_obj, now_lang->usb_text6);
    }
}

void lv_usb_set_enable(bool enable)
{
    if (enable)
    {
        lv_obj_add_state(sw_obj, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(sw_obj, LV_STATE_CHECKED);
    }
}

void lv_usb_set_mode(uint8_t mode)
{
    if (lv_dropdown_get_option_count(mode_obj) > mode)
    {
        lv_dropdown_set_selected(mode_obj, mode);
    }
}

void lv_usb_set_rate(uint8_t rate)
{
    if (lv_dropdown_get_option_count(rate_obj) > rate)
    {
        lv_dropdown_set_selected(rate_obj, rate);
    }
}

void lv_usb_set_bits(uint8_t bits)
{
    if (lv_dropdown_get_option_count(bits_obj) > bits)
    {
        lv_dropdown_set_selected(bits_obj, bits);
    }
}

lv_obj_t *lv_usb_create(lv_obj_t *parent, lv_event_cb_t click, lv_event_cb_t change)
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
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 80);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text3);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 140);

    mode_obj = lv_dropdown_create(obj);
    lv_obj_set_width(mode_obj, 240);
    lv_dropdown_set_options_static(mode_obj, mode);
    lv_obj_align(mode_obj, LV_ALIGN_DEFAULT, 160, 130);
    lv_obj_add_event_cb(mode_obj, mode_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(mode_obj, change, LV_EVENT_VALUE_CHANGED, NULL);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text4);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 200);

    rate_obj = lv_dropdown_create(obj);
    lv_obj_set_width(rate_obj, 240);
    lv_obj_align(rate_obj, LV_ALIGN_DEFAULT, 160, 190);
    lv_obj_add_event_cb(rate_obj, change, LV_EVENT_VALUE_CHANGED, NULL);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text5);
    lv_obj_set_style_text_font(lable, font_18, 0);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 260);

    bits_obj = lv_dropdown_create(obj);
    lv_obj_set_width(bits_obj, 240);
    lv_obj_align(bits_obj, LV_ALIGN_DEFAULT, 160, 250);
    lv_obj_add_event_cb(bits_obj, change, LV_EVENT_VALUE_CHANGED, NULL);

    lable = lv_label_create(obj);
    lv_label_set_text(lable, now_lang->usb_text2);
    lv_obj_align(lable, LV_ALIGN_DEFAULT, 60, 320);

    sw_obj = lv_switch_create(obj);
    lv_obj_add_event_cb(sw_obj, click, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(sw_obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_align(sw_obj, LV_ALIGN_DEFAULT, 160, 315);

    spectrum_obj = lv_spectrum_create(obj);
    lv_obj_align(spectrum_obj, LV_ALIGN_TOP_MID, 0, 480);

    format_obj = lv_label_create(obj);
    lv_obj_set_style_text_font(format_obj, font_16, 0);
    lv_obj_set_style_text_color(format_obj, lv_color_hex(0x8a86b8), 0);
    lv_obj_align(format_obj, LV_ALIGN_TOP_MID, 0, 510);
    lv_label_set_text(format_obj, now_lang->usb_text6);

    change_choise(1);
    lv_dropdown_set_selected(mode_obj, 1);
    lv_dropdown_set_selected(rate_obj, 6);
    lv_dropdown_set_selected(bits_obj, 4);

    return obj;
}