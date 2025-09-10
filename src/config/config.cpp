#include "config.h"

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <pthread.h>

#include "lvgl/src/misc/lv_log.h"
#include <json/json.hpp>

#include "stream/stream_file.h"

using namespace coloraudio::config;
using namespace coloraudio::stream;
using namespace nlohmann;

music_mode_type Config::play_mode = MUSIC_MODE_LOOP;
uint32_t Config::play_index = 0;
std::string Config::play_name;
float Config::play_volume = 20;
main_mode_type Config::main_mode = MAIN_MODE_NONE;
view_mode_type Config::view_mode = VIEW_MAIN;
bool Config::usb_enable = false;
uint8_t Config::usb_mode = 1;
uint8_t Config::usb_rate = 7;
uint8_t Config::usb_bits = 5;

#ifdef BUILD_ARM
bool Config::codec_double = false;
bool Config::wifi_power;
bool Config::wifi_enable;
std::string Config::wifi_ssid;
std::string Config::wifi_pwd;
#endif

static pthread_t save_tid;

static bool need_save;

static void *config_save_run(void *arg)
{
    for (;;)
    {
        if (need_save)
        {
            Config::save_config_run();
            need_save = false;
        }
        usleep(1000000);
    }
}

void Config::load_config()
{
    if (access(MUSIC_CONFIG_NAME, F_OK) == 0)
    {
        FileStream st = FileStream(MUSIC_CONFIG_NAME);
        uint8_t *temp = static_cast<uint8_t *>(malloc(st.get_all_size() + 1));
        st.read(temp, st.get_all_size());
        temp[st.get_all_size()] = 0;

        try
        {
            json j = json::parse(temp);
            json index = j[MUSIC_CONFIG_ID_MUSIC_INDEX];
            json mode = j[MUSIC_CONFIG_ID_MUSIC_MODE];
            json mainmode = j[MUSIC_CONFIG_ID_MAIN_MODE];
            json viewmode = j[MUSIC_CONFIG_ID_VIEW_MODE];
            json name = j[MUSIC_CONFIG_ID_MUISC_NAME];
            json volume = j[MUSIC_CONFIG_ID_VOLUME];
            json usbenable = j[MUSIC_CONFIG_ID_USB_ENABLE];
            json usbmode = j[MUSIC_CONFIG_ID_USB_MODE];
            json usbrate = j[MUSIC_CONFOG_ID_USB_RATE];
            json usbbits = j[MUSIC_CONFOG_ID_USB_BITS];

            if (index.is_number())
            {
                play_index = index.get<uint32_t>();
            }
            if (viewmode.is_number())
            {
                view_mode = viewmode.get<view_mode_type>();
            }
            if (mode.is_number())
            {
                play_mode = mode.get<music_mode_type>();
            }
            if (mainmode.is_number())
            {
                main_mode = mainmode.get<main_mode_type>();
            }
            if (name.is_string())
            {
                play_name = name.get<std::string>();
            }
            if (volume.is_number_float())
            {
                play_volume = volume.get<float>();
            }
            if (usbenable.is_boolean())
            {
                usb_enable = usbenable.get<bool>();
            }
            if (usbmode.is_number())
            {
                usb_mode = usbmode.get<uint8_t>();
            }
            if (usbrate.is_number())
            {
                usb_rate = usbrate.get<uint8_t>();
            }
            if (usbbits.is_number())
            {
                usb_bits = usbbits.get<uint8_t>();
            }
#ifdef BUILD_ARM
            json codec = j[MUSIC_CONFIG_ID_CODEC_DOUBLE];
            json wifipower = j[MUSIC_CONFOG_ID_WIFI_POWER];
            json wifienable = j[MUSIC_CONFOG_ID_WIFI_ENABLE];
            json wifissid = j[MUSIC_CONFOG_ID_WIFI_SSID];
            json wifipwd = j[MUSIC_CONFOG_ID_WIFI_PWD];

            if (codec.is_boolean())
            {
                codec_double = codec.get<bool>();
            }
            if (wifipower.is_boolean())
            {
                wifi_power = wifipower.get<bool>();
            }
            if (wifienable.is_boolean())
            {
                wifi_enable = wifienable.get<bool>();
            }
            if (wifissid.is_string())
            {
                wifi_ssid = wifissid.get<std::string>();
            }
            if (wifipwd.is_string())
            {
                wifi_pwd = wifipwd.get<std::string>();
            }
#endif
        }
        catch (const std::exception &e)
        {
            LV_LOG_ERROR("%s", e.what());
        }

        free(temp);
    }
    else
    {
        save_config();
    }

    void *retval;
    if (save_tid == 0 || pthread_tryjoin_np(save_tid, &retval) == 0)
    {
        int res = pthread_create(&save_tid, NULL, config_save_run, NULL);
        if (res)
        {
            LV_LOG_ERROR("Music play list read thread run fail: %d", res);
        }
    }
}

void Config::save_config()
{
    need_save = true;
}

void Config::save_config_run()
{
    try
    {
        json j = json();
        j[MUSIC_CONFIG_ID_MUSIC_INDEX] = play_index;
        j[MUSIC_CONFIG_ID_MUSIC_MODE] = play_mode;
        j[MUSIC_CONFIG_ID_MAIN_MODE] = main_mode;
        j[MUSIC_CONFIG_ID_MUISC_NAME] = play_name;
        j[MUSIC_CONFIG_ID_VOLUME] = play_volume;
        j[MUSIC_CONFIG_ID_VIEW_MODE] = view_mode;
        j[MUSIC_CONFIG_ID_USB_ENABLE] = usb_enable;
        j[MUSIC_CONFIG_ID_USB_MODE] = usb_mode;
        j[MUSIC_CONFOG_ID_USB_RATE] = usb_rate;
        j[MUSIC_CONFOG_ID_USB_BITS] = usb_bits;
#ifdef BUILD_ARM
        j[MUSIC_CONFIG_ID_CODEC_DOUBLE] = codec_double;
        j[MUSIC_CONFOG_ID_WIFI_POWER] = wifi_power;
        j[MUSIC_CONFOG_ID_WIFI_ENABLE] = wifi_enable;
        j[MUSIC_CONFOG_ID_WIFI_SSID] = wifi_ssid;
        j[MUSIC_CONFOG_ID_WIFI_PWD] = wifi_pwd;
#endif

        std::string res = j.dump();

        FILE *file = fopen(MUSIC_CONFIG_NAME, "w");
        fwrite(res.c_str(), res.length(), 1, file);
        fclose(file);
    }
    catch (const std::exception &e)
    {
        LV_LOG_ERROR("%s", e.what());
    }
}