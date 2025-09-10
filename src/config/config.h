#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include <string>

#include "music/music.h"
#include "main.h"
#include "ui/ui.h"

#define MUSIC_CONFIG_NAME "config.json"

#define MUSIC_CONFIG_ID_MAIN_MODE "main_mode"
#define MUSIC_CONFIG_ID_VIEW_MODE "view_mode"

#define MUSIC_CONFIG_ID_MUSIC_MODE "music_mode"
#define MUSIC_CONFIG_ID_MUISC_NAME "music_name"
#define MUSIC_CONFIG_ID_MUSIC_INDEX "music_index"

#define MUSIC_CONFIG_ID_CODEC_DOUBLE "codec_sec"

#define MUSIC_CONFIG_ID_USB_ENABLE "usb_enable"
#define MUSIC_CONFIG_ID_USB_MODE "usb_mode"
#define MUSIC_CONFOG_ID_USB_RATE "usb_rate"
#define MUSIC_CONFOG_ID_USB_BITS "usb_bits"

#define MUSIC_CONFOG_ID_WIFI_POWER "wifi_power"
#define MUSIC_CONFOG_ID_WIFI_ENABLE "wifi_enable"
#define MUSIC_CONFOG_ID_WIFI_SSID "wifi_ssid"
#define MUSIC_CONFOG_ID_WIFI_PWD "wifi_pwd"

#define MUSIC_CONFIG_ID_VOLUME "volume"

namespace coloraudio::config
{
    class Config
    {
    private:
        static music_mode_type play_mode;
        static main_mode_type main_mode;
        static view_mode_type view_mode;
        static uint32_t play_index;
        static std::string play_name;
        static float play_volume;
        static bool usb_enable;
        static uint8_t usb_mode;
        static uint8_t usb_rate;
        static uint8_t usb_bits;

#ifdef BUILD_ARM
        static bool codec_double;
        static bool wifi_power;
        static bool wifi_enable;
        static std::string wifi_ssid;
        static std::string wifi_pwd;
#endif

    public:
        static void load_config();
        static void save_config();
        static void save_config_run();

        static void set_config_view_mode(view_mode_type mode)
        {
            view_mode = mode;
        }

        static void set_config_volume(float volume)
        {
            play_volume = volume;
        }

        static void set_config_music_code(music_mode_type mode)
        {
            play_mode = mode;
        }

        static void set_config_main_mode(main_mode_type mode)
        {
            main_mode = mode;
        }

        static void set_config_music_index(uint32_t index)
        {
            play_index = index;
        }

        static void set_config_music_name(std::string &name)
        {
            play_name = name;
        }

        static void set_config_usb_enable(bool enable)
        {
            usb_enable = enable;
        }

        static void set_config_usb_mode(uint8_t mode)
        {
            usb_mode = mode;
        }

        static void set_config_usb_rate(uint8_t rate)
        {
            usb_rate = rate;
        }

        static void set_config_usb_bits(uint8_t bits)
        {
            usb_bits = bits;
        }

#ifdef BUILD_ARM
        static void set_config_codec_double(bool enable)
        {
            codec_double = enable;
        }

        static void set_config_wifi_power(bool enable)
        {
            wifi_power = enable;
        }

        static void set_config_wifi_enable(bool enable)
        {
            wifi_enable = enable;
        }

        static void set_config_wifi_ssid(std::string& ssid)
        {
            wifi_ssid = ssid;
        }

        static void set_config_wifi_pwd(std::string& pwd)
        {
            wifi_pwd = pwd;
        }
#endif

        static music_mode_type get_config_music_mode()
        {
            return play_mode;
        }

        static main_mode_type get_config_main_mode()
        {
            return main_mode;
        }

        static view_mode_type get_config_view_mode()
        {
            return view_mode;
        }

        static uint32_t get_config_music_index()
        {
            return play_index;
        }

        static std::string &get_config_music_name()
        {
            return play_name;
        }

        static float get_config_volume()
        {
            return play_volume;
        }

        static bool get_config_usb_enable()
        {
            return usb_enable;
        }

        static uint8_t get_config_usb_mode()
        {
            return usb_mode;
        }

        static uint8_t get_config_usb_rate()
        {
            return usb_rate;
        }

        static uint8_t get_config_usb_bits()
        {
            return usb_bits;
        }

#ifdef BUILD_ARM
        static bool get_config_codec_double()
        {
            return codec_double;
        }

        static bool get_config_wifi_power()
        {
            return wifi_power;
        }

        static bool get_config_wifi_enable()
        {
            return wifi_enable;
        }

        static std::string &get_config_wifi_ssid()
        {
            return wifi_ssid;
        }

        static std::string &get_config_wifi_pwd()
        {
            return wifi_pwd;
        }
#endif
    };
}

#endif // __CONFIG_H__