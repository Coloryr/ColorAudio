#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../music/player_info.h"
#include "../main.h"
#include "../ui/ui.h"

#include <stdint.h>
#include <string>

#define MUSIC_CONFIG_NAME "config.json"

#define MUSIC_CONFOG_ID_MAIN_MODE "main_mode"
#define MUSIC_CONFOG_ID_VIEW_MODE "view_mode"

#define MUSIC_CONFOG_ID_MUSIC_MODE "music_mode"
#define MUSIC_CONFOG_ID_MUISC_NAME "music_name"
#define MUSIC_CONFOG_ID_MUSIC_INDEX "music_index"

#define MUSIC_CONFOG_ID_VOLUME "volume"

namespace ColorAudio
{
    class config
    {
    private:
        static music_mode_type play_mode;
        static main_mode_type main_mode;
        static view_mode_type view_mode;
        static uint32_t play_index;
        static std::string play_name;
        static float play_volume;

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
    };

}

#endif // __CONFIG_H__