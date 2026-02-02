#include <stdint.h>

#include "lvgl.h"

#include "music_view.h"
#include "info_view.h"
#include "main_view.h"
#include "input_view.h"
#include "ble_view.h"
#include "usb_view.h"
#include "header_view.h"
#include "setting_view.h"
#include "lang.h"
#include "config/config.h"
#include "music/music_player.h"
#include "sound/sound.h"

#include "ui.h"

using namespace coloraudio::config;

static view_mode_type now_type = VIEW_MAIN;

static void change_view(view_mode_type type)
{
    if (now_type == VIEW_MAIN)
    {
        view_main_set_display(false);
    }
    else if (now_type == VIEW_MUSIC)
    {
        view_music_set_display(false);
    }
    else if (now_type == VIEW_BLE)
    {
        view_ble_set_display(false);
    }
    else if (now_type == VIEW_USB)
    {
        view_usb_set_display(false);
    }
    else if (now_type == VIEW_SETTING)
    {
        view_setting_set_display(false);
    }

    if (type == VIEW_MAIN)
    {
        view_main_set_display(true);
        view_main_set_header();
    }
    else if (type == VIEW_MUSIC)
    {
        view_music_set_display(true);
        view_music_set_header();
    }
    else if (type == VIEW_BLE)
    {
        view_ble_set_display(true);
        view_ble_set_header();
    }
    else if (type == VIEW_USB)
    {
        view_usb_set_display(true);
        view_usb_set_header();
    }
    else if (type == VIEW_SETTING)
    {
        view_setting_set_display(true);
        view_setting_set_header();
    }

    now_type = type;
}

void view_init()
{
    lv_log("view init");

    lang_init();

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    // music
    view_main_create(lv_screen_active());
    view_music_create(lv_screen_active());
    // ble
    view_ble_create(lv_screen_active());
    // usb
    view_usb_create(lv_screen_active());
    // setting
    view_setting_create(lv_screen_active());
    // top level
    view_input_create(lv_screen_active());
    view_top_info_create(lv_screen_active());
    // header
    view_header_create(lv_screen_active());

    view_music_set_display(false);
    view_ble_set_display(false);
    view_usb_set_display(false);
    view_setting_set_display(false);

    change_view(Config::get_config_view_mode());
}

view_mode_type get_view_mode()
{
    return now_type;
}

void view_jump(view_mode_type type)
{
    if (now_type == type)
    {
        return;
    }

    change_view(type);

    Config::set_config_view_mode(now_type);
    Config::save_config();
}

void view_set_fft_data(uint16_t size, float *value)
{
    if (now_type == VIEW_MUSIC)
    {
        for (uint16_t i = 0; i < size; i++)
        {
            view_music_set_fft_data(i, (uint16_t)(value[i] * 20));
        }
    }
    else if (now_type == VIEW_USB)
    {
        for (uint16_t i = 0; i < size; i++)
        {
            view_usb_set_fft_data(i, (uint16_t)(value[i] * 20));
        }
    }
    else if (now_type == VIEW_BLE)
    {
        for (uint16_t i = 0; i < size; i++)
        {
            view_ble_set_fft_data(i, (uint16_t)(value[i] * 20));
        }
    }
}

void view_tick()
{
    if (now_type == VIEW_MUSIC)
    {
        view_music_tick();
    }
    else if (now_type == VIEW_BLE)
    {
        view_ble_tick();
    }
    else if (now_type == VIEW_USB)
    {
        view_usb_tick();
    }
}
