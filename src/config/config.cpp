#include "config.h"

#include "../stream/stream_file.h"

#include "../lvgl/src/misc/lv_log.h"
#include <json/json.hpp>

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <pthread.h>

using namespace ColorAudio;
using namespace nlohmann;

music_mode_type config::play_mode = MUSIC_MODE_LOOP;
uint32_t config::play_index = 0;
std::string config::play_name;
float config::play_volume = 20;
main_mode_type config::main_mode = MAIN_MODE_NONE;
view_mode_type config::view_mode = VIEW_MAIN;

static pthread_t save_tid;

static bool need_save;

static void *config_save(void *arg)
{
    for (;;)
    {
        if (need_save)
        {
            config::save_config_run();
            need_save = false;
        }
        usleep(1000000);
    }
}

void config::load_config()
{
    if (access(MUSIC_CONFIG_NAME, F_OK) == 0)
    {
        StreamFile st = StreamFile(MUSIC_CONFIG_NAME);
        uint8_t *temp = static_cast<uint8_t *>(malloc(st.get_all_size() + 1));
        st.read(temp, st.get_all_size());
        temp[st.get_all_size()] = 0;

        try
        {
            json j = json::parse(temp);
            json index = j[MUSIC_CONFOG_ID_MUSIC_INDEX];
            json mode = j[MUSIC_CONFOG_ID_MUSIC_MODE];
            json mainmode = j[MUSIC_CONFOG_ID_MAIN_MODE];
            json viewmode = j[MUSIC_CONFOG_ID_VIEW_MODE];
            json name = j[MUSIC_CONFOG_ID_MUISC_NAME];
            json volume = j[MUSIC_CONFOG_ID_VOLUME];

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
        int res = pthread_create(&save_tid, NULL, config_save, NULL);
        if (res)
        {
            LV_LOG_ERROR("Music play list read thread run fail: %d", res);
        }
    }
}

void config::save_config()
{
    need_save = true;
}

void config::save_config_run()
{
    try
    {
        json j = json();
        j[MUSIC_CONFOG_ID_MUSIC_INDEX] = play_index;
        j[MUSIC_CONFOG_ID_MUSIC_MODE] = play_mode;
        j[MUSIC_CONFOG_ID_MAIN_MODE] = main_mode;
        j[MUSIC_CONFOG_ID_MUISC_NAME] = play_name;
        j[MUSIC_CONFOG_ID_VOLUME] = play_volume;
        j[MUSIC_CONFOG_ID_VIEW_MODE] = view_mode;

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