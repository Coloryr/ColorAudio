#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../music/player_info.h"

#include <stdint.h>
#include <string>

#define MUSIC_CONFIG_NAME "config.json"

namespace ColorAudio
{
    class config
    {
    private:
        static music_mode play_mode;
        static uint32_t play_index;
        static std::string play_name;
        static float play_volume;

    public:
        static void load_config();
        static void save_config();
        static void save_config_run();

        static void set_config_volume(float volume)
        {
            play_volume = volume;
        }

        static void set_config_music_code(music_mode mode)
        {
            play_mode = mode;
        }

        static void set_config_music_index(uint32_t index)
        {
            play_index = index;
        }

        static void set_config_music_name(std::string &name)
        {
            play_name = name;
        }

        static music_mode get_config_music_mode()
        {
            return play_mode;
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
    };

}

#endif // __CONFIG_H__