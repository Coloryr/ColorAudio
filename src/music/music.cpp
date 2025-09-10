#include "music.h"

#include <stdint.h>
#include <pthread.h>
#include <deque>

#include "lyric.h"
#include "local_music.h"
#include "music_player.h"

#include "163/music163.h"

#include "ui/lang.h"
#include "ui/ui.h"
#include "ui/music_view.h"
#include "ui/info_view.h"
#include "config/config.h"
#include "common/utils.h"
#include "common/utilspp.h"
#include "io/gpio.h"

using namespace coloraudio::lyric;
using namespace coloraudio::stream;
using namespace coloraudio::config;

static music_run_type music_run = MUSIC_RUN_UNKNOW;

static pthread_t rtid;

static uint32_t jump_index = UINT32_MAX;

static std::deque<uint32_t> play_last_stack;

static bool is_close;

music_type play_music_type = MUSIC_TYPE_UNKNOW;
music_state play_state = MUSIC_STATE_UNKNOW;
music_mode_type play_music_mode = MUSIC_MODE_LOOP;

uint32_t play_music_bps;
uint32_t play_now_index;
uint32_t play_list_count;

float target_time = 0;
float time_all = 0;
float time_now = 0;

music_type music_test_type(BaseStream *st)
{
    uint8_t buffer[8];
    st->peek(buffer, sizeof(buffer));

    if (buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F')
    {
        return MUSIC_TYPE_WAV;
    }
    else if (buffer[0] == 'f' && buffer[1] == 'L' && buffer[2] == 'a' && buffer[3] == 'C')
    {
        return MUSIC_TYPE_FLAC;
    }
    else if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
    {
        return MUSIC_TYPE_MP3;
    }
    else if (buffer[0] == 0xFF && buffer[1] == 0xFB)
    {
        return MUSIC_TYPE_MP3;
    }
    else if (buffer[0] == 'C' && buffer[1] == 'T' && buffer[2] == 'E' && buffer[3] == 'N' &&
             buffer[4] == 'F' && buffer[5] == 'D' && buffer[6] == 'A' && buffer[7] == 'M')
    {
        return MUSIC_TYPE_NCM;
    }

    return MUSIC_TYPE_UNKNOW;
}

void music_start()
{
    view_music_set_check(play_now_index, true);
}

void music_get_lyric(std::string &comment)
{
#ifdef BUILD_ARM
    if (get_wireless_power() == false)
    {
        view_music_set_lyric_state(LYRIC_FAIL);
        return;
    }
#endif
    try
    {
        if (comment.find("163 key(Don't modify):") == 0)
        {
            view_music_set_lyric_state(LYRIC_GET);
            LyricParser *data, *tr_data;
            if (music_lyric_163(comment, &data, &tr_data))
            {
                view_music_set_lyric(data, tr_data);
            }
            else
            {
                view_music_set_lyric_state(LYRIC_NONE);
            }
        }
    }
    catch (const std::exception &e)
    {
        LV_LOG_ERROR("%s", e.what());
        view_music_set_lyric_state(LYRIC_FAIL);
    }
}

void music_end()
{
    view_music_set_lyric_state(LYRIC_CLEAR);
    play_clear();

    view_music_set_check(play_now_index, false);

    if (is_close)
    {
        play_jump_index_clear();
        is_close = false;
    }
    else
    {
        if (have_jump_index())
        {
            play_now_index = get_jump_index();
            play_jump_index_clear();
        }
        else
        {
            music_next();
        }
    }

    // 清理指令
    play_set_command(MUSIC_COMMAND_UNKNOW);
}

void music_next()
{
    if (play_get_command() == MUSIC_COMMAND_NEXT)
    {
        if (play_music_mode == MUSIC_MODE_RND)
        {
            play_last_stack.push_front(play_now_index);
            uint32_t next_value;
            bool is_have = false;
            do
            {
                is_have = false;
                next_value = read_random() % play_list_count;
                for (auto it = play_last_stack.rbegin(); it != play_last_stack.rend(); ++it)
                {
                    if (*it == next_value)
                    {
                        is_have = true;
                        break;
                    }
                }
            } while (is_have);
            play_now_index = next_value;
            if (play_last_stack.size() > play_list_count / 10)
            {
                play_last_stack.pop_back();
            }
        }
        else if (play_music_mode == MUSIC_MODE_LOOP)
        {
            play_now_index++;
            if (play_now_index >= play_list_count)
            {
                play_now_index = 0;
            }
        }
    }
    else if (play_get_command() == MUSIC_COMMAND_LAST)
    {
        if (play_music_mode == MUSIC_MODE_RND)
        {
            if (play_last_stack.size() == 0)
            {
                goto last_go;
            }
            else
            {
                play_now_index = play_last_stack.front();
                play_last_stack.pop_front();
            }
        }
        else if (play_music_mode == MUSIC_MODE_LOOP)
        {
        last_go:
            if (play_now_index == 0)
            {
                play_now_index = play_list_count - 1;
            }
            else
            {
                play_now_index--;
            }
        }
    }
}

void play_jump_index(uint32_t index)
{
    if (index >= play_list_count)
    {
        jump_index = play_list_count - 1;
    }
    else
    {
        jump_index = index;
    }

    play_state = MUSIC_STATE_STOP;
}

bool have_jump_index()
{
    return jump_index != UINT32_MAX;
}

void play_jump_index_clear()
{
    jump_index = UINT32_MAX;
}

uint32_t get_jump_index()
{
    return jump_index;
}

void music_go_local()
{
    music_run = MUSIC_RUN_LOCAL;
}

void music_init()
{
    play_last_stack.clear();

    play_music_mode = Config::get_config_music_mode();

    local_music_init();
}

void music_close()
{
    is_close = true;
    play_set_command(MUSIC_COMMAND_STOP);
}

music_run_type get_music_run()
{
    return music_run;
}

void music_run_loop()
{
    if (local_music_scan_now)
    {
        if (get_view_mode() == VIEW_MUSIC && !view_top_info_is_display())
        {
            view_top_info_display(now_lang->music_text9);
        }
    }
    else
    {
        if (get_view_mode() == VIEW_MUSIC && view_top_info_is_display())
        {
            view_top_info_close();
        }

        pthread_mutex_lock(&play_mutex);
        play_item *item = play_list_get_now();

        if (start_with(item->path, "http"))
        {
        }
        else
        {
            local_music_run(item);
            std::string path = std::string(item->path);
            std::string name = get_file_name(path);
            Config::set_config_music_name(name);
            Config::set_config_music_index(play_now_index);
            Config::save_config();
        }

        usleep(1000);

        // 等待播放结束
        pthread_mutex_lock(&play_mutex);

        music_end();

        pthread_mutex_unlock(&play_mutex);
    }

    // if (music_run == MUSIC_RUN_LOCAL)
    // {

    // }
}