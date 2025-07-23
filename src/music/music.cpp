#include "music.h"

#include "lyric.h"
#include "local_music.h"

#include "../player/player.h"
#include "../net/music_api.h"
#include "../ui/music_view.h"
#include "../ui/info_view.h"
#include "../config/config.h"
#include "../common/utils.h"

#include <stdint.h>
#include <pthread.h>
#include <deque>
#include <json/json.hpp>

using namespace ColorAudio;

static music_run_type music_run = MUSIC_RUN_UNKNOW;

static pthread_t rtid;

static uint32_t jump_index = UINT32_MAX;

static std::deque<uint32_t> play_last_stack;

static void *music_run_loop(void *arg)
{
    for (;;)
    {
        usleep(500);
        if (music_run == MUSIC_RUN_LOCAL)
        {
            if (local_music_scan_now)
            {
                if (!view_top_info_is_display())
                {
                    view_top_info_display("正在扫描音乐");
                }
            }
            else
            {
                if (view_top_info_is_display())
                {
                    view_top_info_close();
                }
                local_music_run();
            }
        }
    }
}

void music_lyric_163(uint64_t id)
{
    json j = api_lyric_music_new(id);

    std::string lyric, tlyric;
    if (api_music_get_lyric_new(j, lyric, tlyric))
    {
        LyricParser *data = new LyricParser(lyric);
        LyricParser *tr_data = new LyricParser(tlyric);

        view_music_set_lyric(data, tr_data);
    }
    else
    {
        j = api_lyric_music(id);

        std::string lyric, tlyric;
        if (api_music_get_lyric(j, lyric, tlyric))
        {
            LyricParser *data = new LyricParser(lyric);
            LyricParser *tr_data = new LyricParser(tlyric);

            view_music_set_lyric(data, tr_data);
        }
        else
        {
            view_music_set_lyric_state(LYRIC_NONE);
        }
    }
}

void music_start()
{
    view_music_set_check(play_now_index, true);
}

void music_end()
{
    view_music_set_lyric_state(LYRIC_CLEAR);
    play_clear();

    view_music_set_check(play_now_index, false);

    if (have_jump_index())
    {
        play_now_index = get_jump_index();
        play_jump_index_clear();
    }
    else
    {
        music_next();
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

void music_go_net()
{
    music_run = MUSIC_RUN_NET;
}

void music_init()
{
    play_last_stack.clear();

    play_music_mode = config::get_config_music_mode();

    music_run = MUSIC_RUN_LOCAL;

    int res = pthread_create(&rtid, NULL, music_run_loop, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music thread run fail: %d", res);
    }
}
