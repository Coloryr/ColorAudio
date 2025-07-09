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

music_mode config::play_mode = MUSIC_MODE_LOOP;
uint32_t config::play_index = 0;
std::string config::play_name;
float config::play_volume = 20;

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
            json index = j["index"];
            json mode = j["mode"];
            json name = j["name"];
            json volume = j["volume"];

            if (index.is_number())
            {
                play_index = index.get<uint32_t>();
            }
            if (mode.is_number())
            {
                play_mode = mode.get<music_mode>();
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
        j["index"] = play_index;
        j["mode"] = play_mode;
        j["name"] = play_name;
        j["volume"] = play_volume;

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