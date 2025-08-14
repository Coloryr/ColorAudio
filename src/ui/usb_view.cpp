#include "lvgl.h"

#include "header_view.h"
#include "view/view_usb.h"
#include "config/config.h"
#include "sound/sound.h"
#include "usb/usb_audio.h"

#include "usb_view.h"

using namespace coloraudio::config;

static lv_obj_t *view_obj;
static bool update, is_connect;

const static char *rate_arg[2][7] = {
    {"44100", "48000", "96000", "44100,48000", "44100,48000,96000"},
    {"44100", "48000", "96000", "192000", "44100,48000", "44100,48000,96000", "44100,48000,96000,192000"}};
const static char *bits_arg[] = {"2", "3", "4", "2,3", "2,3,4"};

static void change_event(lv_event_t *change)
{
    Config::set_config_usb_mode(lv_usb_get_mode());
    Config::set_config_usb_rate(lv_usb_get_rate());
    Config::set_config_usb_bits(lv_usb_get_bits());
    Config::save_config();
}

static void click_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *obj = lv_event_get_target_obj(event);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        bool chk = lv_obj_get_state(obj) & LV_STATE_CHECKED;
        if (chk)
        {
            uint32_t mode = lv_usb_get_mode();
            usb_audio_set_mode(mode);
            usb_audio_set_rate(rate_arg[mode][lv_usb_get_rate()]);
            usb_audio_set_bits(bits_arg[lv_usb_get_bits()]);
            usb_audio_change(true);
            lv_usb_lock(true);
        }
        else
        {
            usb_audio_change(false);
            lv_usb_lock(false);
        }

        Config::set_config_usb_enable(chk);
        Config::save_config();
    }
}

static void timer(lv_timer_t *timer)
{
    if (update)
    {
#ifdef BUILD_ARM
        lv_usb_set_format(is_connect, pcm_now_rate, pcm_now_format_size, pcm_now_db);
#endif
        update = false;
    }
}

void view_usb_set_header()
{
    view_header_move(view_obj);
    view_header_back_display(true, false);
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

void view_usb_set_fft_data(uint16_t index, uint16_t value)
{
    lv_usb_set_fft_data(index, value);
}

void view_usb_tick()
{
    if (is_connect)
    {
        lv_usb_fft_load();
    }
}

void view_usb_update(bool connect)
{
    is_connect = connect;
    update = true;
    if (!is_connect)
    {
        for (uint16_t i = 0; i < 20; i++)
        {
            lv_usb_set_fft_data(i, 0);
        }
    }
}

void view_usb_create(lv_obj_t *parent)
{
    view_obj = lv_usb_create(parent, click_event, change_event);
    lv_timer_create(timer, 500, NULL);

    lv_usb_set_mode(Config::get_config_usb_mode());
    lv_usb_set_rate(Config::get_config_usb_rate());
    lv_usb_set_bits(Config::get_config_usb_bits());
    if (Config::get_config_usb_enable())
    {
        lv_usb_set_enable(true);
        lv_usb_lock(true);
        uint32_t mode = lv_usb_get_mode();
        usb_audio_set_mode(mode);
        usb_audio_set_rate(rate_arg[mode][lv_usb_get_rate()]);
        usb_audio_set_bits(bits_arg[lv_usb_get_bits()]);
    }
}