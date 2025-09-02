#include <vector>

#include "lvgl.h"

#include "header_view.h"
#include "input_view.h"
#include "lang.h"
#include "view/view_setting.h"
#include "main.h"
#include "sound/sound.h"
#include "config/config.h"
#include "wireless/wifi.h"
#include "io/gpio.h"

#include "setting_view.h"

using namespace coloraudio::config;

static lv_obj_t *view_obj;
static std::vector<wifi_item_t> wifi_list;
static bool update_wifi;

static void wifi_power_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
#ifdef BUILD_ARM
    Config::set_config_wifi_power(enabled);
    Config::save_config();

    add_work(MAIN_WORK_WIFI_POWER, NULL);
#endif
}
#include <vector>
#include <algorithm>
static void wifi_state_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
#ifdef BUILD_ARM
    Config::set_config_wifi_enable(enabled);
    Config::save_config();

    add_work(MAIN_WORK_WIFI_ENABLE, NULL);
#endif
}

static void codec_switch_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
#ifdef BUILD_ARM
    Config::set_config_codec_double(enabled);
    Config::save_config();

    alsa_codec_double_change();
#endif
}

static void scan_wifi_handler(lv_event_t *e)
{
    add_work(MAIN_WORK_WIFI_SCAN, NULL);
}

static void wifi_connect(bool cancel, void *data)
{
    wifi_connect_t *item = static_cast<wifi_connect_t *>(data);

    if (!cancel)
    {
        add_work(MAIN_WORK_WIFI_CONNECT, item);
    }
    else
    {
        delete item;
    }
}

static void wifi_list_handler(lv_event_t *e)
{
    lv_obj_t *drop = lv_event_get_target_obj(e);
    wifi_connect_t *item = new wifi_connect_t();
    lv_dropdown_get_selected_str(drop, item->ssid, sizeof(item->ssid));
    view_input_show(item->pwd, sizeof(item->pwd), wifi_connect, now_lang->setting_text14, item);
}

static void wifi_disconnect_handler(lv_event_t *e)
{
    add_work(MAIN_WORK_WIFI_DISCONNECT, NULL);
}

static void timer(lv_timer_t *timer)
{
    if (!wifi_have_device() || !wifi_is_wpa_supplicant_running())
    {
        lv_setting_update_wifi(now_lang->setting_text12);
        return;
    }

    wifi_state state;
    std::string ssid;
    if (wifi_get_state(&state, ssid))
    {
        if (state == WIFI_STATE_DISCONNECTED)
        {
            lv_setting_update_wifi(now_lang->setting_text4);
        }
        else if (state == WIFI_STATE_COMPLETED)
        {
            char temp[256];
            sprintf(temp, now_lang->setting_text17, ssid.c_str());
            lv_setting_update_wifi(temp);
        }
        else
        {
            lv_setting_update_wifi(now_lang->setting_text18);
        }
    }

    if (update_wifi)
    {
        lv_setting_wifi_clear_list();
        for (auto &item1 : wifi_list)
        {
            lv_setting_wifi_add_list(item1.ssid.c_str());
        }
        update_wifi = false;
    }
}

void view_setting_set_header()
{
    view_header_move(view_obj);
    view_header_back_display(true, true);
}

void view_setting_wifi_list(std::vector<wifi_item_t> &list)
{
    wifi_list.clear();

    std::copy_if(list.begin(), list.end(), std::back_inserter(wifi_list),
                 [](const wifi_item_t &item)
                 { return !item.ssid.empty(); });

    update_wifi = true;
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
                                 scan_wifi_handler, codec_switch_handler, wifi_list_handler, wifi_disconnect_handler);
    lv_timer_create(timer, 500, NULL);
    lv_setting_set_wifi(false, false);
#ifdef BUILD_ARM
    lv_setting_set_codec(Config::get_config_codec_double());

    lv_setting_set_wifi(get_wireless_power(), wifi_is_wpa_supplicant_running());
#endif
}