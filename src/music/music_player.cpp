#include <stdint.h>
#include <pthread.h>
#include <string>
#include <map>
#include <execution>

#include "lvgl/src/misc/lv_log.h"
#include <boost/locale.hpp>
#include <boost/container/vector.hpp>

#include "mp3/mp3_id3.h"
#include "music.h"
#include "decoder/decoder.h"
#include "decoder/decoder_flac.h"
#include "decoder/decoder_mp3.h"
#include "sound/sound.h"
#include "stream/stream.h"
#include "ui/music_view.h"
#include "config/config.h"
#include "common/utilspp.h"

#include "music_player.h"

using namespace coloraudio::common;
using namespace coloraudio::decoder;
using namespace coloraudio::stream;
using namespace coloraudio::config;

static pthread_t tid;
static music_command play_now_command = MUSIC_COMMAND_UNKNOW;
static float jump_time = 0;
static boost::container::vector<play_item *> play_list;
static DataItem str_map = DataItem(DEFAULT_STR_MAP);
static uint32_t str_map_index = 0;

std::string play_title;
std::string play_album;
std::string play_auther;
DataItem *play_image;

BaseStream *play_st;

pthread_mutex_t play_mutex;
pthread_cond_t play_start;

bool play_need_seek;

static void *play_run(void *arg)
{
    for (;;)
    {
        play_state = MUSIC_STATE_STOP;

        pthread_mutex_lock(&play_mutex);
        while (play_st == NULL)
        {
            pthread_cond_wait(&play_start, &play_mutex);
        }

        play_music_type = music_test_type(play_st);
        if (play_music_type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            delete play_st;
            continue;
        }

        alsa_clear();
        alsa_reset();

        time_now = 0;

        play_state = MUSIC_STATE_PLAY;

        view_music_update_state();

        Decoder *play_decoder;

        uint32_t start_pos = play_st->get_pos();
    play:
        switch (play_music_type)
        {
        case MUSIC_TYPE_MP3:
            LV_LOG_USER("Start play mp3");
            play_decoder = new DecoderMp3(play_st);
            break;
        case MUSIC_TYPE_FLAC:
            LV_LOG_USER("Start play flac");
            play_decoder = new DecoderFlac(play_st);
            break;
        default:
            break;
        }

        if (!play_decoder->decode_start())
        {
            view_music_update_state();
            LV_LOG_USER("play decoder run fail");
        }
        delete play_decoder;

        if (target_time > 0 && play_now_command == MUSIC_COMMAND_UNKNOW)
        {
            alsa_clear();
            play_st->seek(start_pos, SEEK_SET);
            play_need_seek = false;
            goto play;
        }

        target_time = 0;

        view_music_clear_info();

        delete play_st;
        play_st = NULL;

        // 自动下一首
        if (play_state != MUSIC_STATE_STOP)
        {
            play_now_command = MUSIC_COMMAND_NEXT;
        }
        play_state = MUSIC_STATE_STOP;

        pthread_mutex_unlock(&play_mutex);
    }
}

void play_update_text(std::string text, music_info_type type)
{
    switch (type)
    {
    case MUSIC_INFO_TITLE:
        play_title = text;
        break;
    case MUSIC_INFO_AUTHER:
        play_auther = text;
        break;
    case MUSIC_INFO_ALBUM:
        play_album = text;
        break;
    default:
        break;
    }
}

void play_clear()
{
    play_title.clear();
    play_album.clear();
    play_auther.clear();

    play_update_image(nullptr, MUSIC_INFO_IMAGE);

    view_music_update_info();
    view_music_update_img();
}

void play_update_image(DataItem *data, music_info_type type)
{
    switch (type)
    {
    case MUSIC_INFO_IMAGE:
        if (play_image)
        {
            delete play_image;
        }
        play_image = data;
        break;
    default:
        break;
    }
}

void play_init()
{
    pthread_mutex_init(&play_mutex, NULL);
    pthread_cond_init(&play_start, NULL);

    int res = pthread_create(&tid, NULL, play_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play thread run fail: %d", res);
    }
    pthread_setname_np(tid, "music play loop");
}

music_command get_play_command()
{
    return play_now_command;
}

bool play_set_command(music_command command)
{
    switch (command)
    {
    case MUSIC_COMMAND_PLAY:
        if (play_state == MUSIC_STATE_PAUSE)
        {
            play_state = MUSIC_STATE_PLAY;
            return true;
        }
    case MUSIC_COMMAND_PAUSE:
        if (play_state == MUSIC_STATE_PLAY)
        {
            play_state = MUSIC_STATE_PAUSE;
            return true;
        }
    case MUSIC_COMMAND_STOP:
        play_state = MUSIC_STATE_STOP;
        return true;
    case MUSIC_COMMAND_NEXT:
    case MUSIC_COMMAND_LAST:
        play_state = MUSIC_STATE_STOP;
        play_now_command = command;
        return true;
    case MUSIC_COMMAND_CHANGE_MODE:
        play_music_mode = static_cast<music_mode_type>((play_music_mode + 1) % 2);
        Config::set_config_music_code(play_music_mode);
        Config::save_config();
        view_music_update_state();
        return true;
    case MUSIC_COMMAND_UNKNOW:
        play_now_command = MUSIC_COMMAND_UNKNOW;
        return true;
    default:
        return false;
    }
}

void play_jump_time(float time)
{
    play_need_seek = true;
    target_time = time / 1000;
    jump_time = target_time;
}

void play_jump_end()
{
    play_need_seek = false;
    time_now = jump_time - target_time;
    jump_time = 0;
    target_time = 0;
}

music_command play_get_command()
{
    return play_now_command;
}

void play_list_close()
{
    for (const auto &it : play_list)
    {
        delete it;
    }

    play_list.clear();
    play_list_count = 0;
}

void play_list_add_item(std::string &path, std::string &title, std::string &auther, float time)
{
    play_item *item = new play_item();
    item->time = time;

    if (path.length() + str_map_index >= str_map.get_size())
    {
        str_map.resize(str_map.get_size() + DEFAULT_STR_MAP);
    }
    item->path = reinterpret_cast<char *>(str_map.get_data()) + str_map_index;
    memcpy(item->path, path.c_str(), path.length());
    str_map_index += path.length();
    str_map.get_data()[str_map_index] = 0;
    str_map_index += 1;

    if (title.length() + str_map_index >= str_map.get_size())
    {
        str_map.resize(str_map.get_size() + DEFAULT_STR_MAP);
    }
    item->title = reinterpret_cast<char *>(str_map.get_data()) + str_map_index;
    memcpy(item->title, title.c_str(), title.length());
    str_map_index += title.length();
    str_map.get_data()[str_map_index] = 0;
    str_map_index += 1;

    if (auther.length() + str_map_index >= str_map.get_size())
    {
        str_map.resize(str_map.get_size() + DEFAULT_STR_MAP);
    }
    item->auther = reinterpret_cast<char *>(str_map.get_data()) + str_map_index;
    memcpy(item->auther, auther.c_str(), auther.length());
    str_map_index += auther.length();
    str_map.get_data()[str_map_index] = 0;
    str_map_index += 1;

    LV_LOG_USER("add music: %s", item->path);

    play_list.push_back(item);
}

bool play_list_empty()
{
    return play_list.empty();
}

void play_list_read_done()
{
    for (const auto &item : play_list)
    {
        item->index = play_list_count++;
    }

    std::string name = Config::get_config_music_name();
    uint32_t index = Config::get_config_music_index();

    play_now_index = 0;

    if (play_list.size() > index)
    {
        play_item *item = play_list[index];
        if (end_with(item->path, name))
        {
            play_now_index = index;
        }
        else
        {
            for (const auto &item : play_list)
            {
                if (end_with(item->path, name))
                {
                    play_now_index = item->index;
                    break;
                }
            }
        }
    }
}

play_item *play_list_get_now()
{
    return play_list[play_now_index];
}

void play_list_sort_by_pinyin()
{
    boost::locale::generator gen;
    std::locale loc = gen.generate("zh_CN.UTF-8");
    std::collate<wchar_t> const &coll = std::use_facet<std::collate<wchar_t>>(loc);

    std::vector<std::map<std::string, std::wstring> *> cache_list;

    auto compare = [&](play_item *a, play_item *b)
    {
        static thread_local std::map<std::string, std::wstring> cache;
        bool find = false;
        for (auto &item : cache_list)
        {
            if (item == &cache)
            {
                find = true;
            }
        }
        if (!find)
        {
            LV_LOG_USER("add cache: %p", &cache);
            cache_list.push_back(&cache);
        }

        auto get_wstring = [](const std::string &str) -> std::wstring
        {
            auto it = cache.find(str);
            if (it != cache.end())
                return it->second;

            std::wstring result = boost::locale::conv::to_utf<wchar_t>(str, "UTF-8");
            cache[str] = result;
            return result;
        };

        std::wstring wa = get_wstring(a->auther);
        std::wstring wb = get_wstring(b->auther);

        return coll.compare(wa.data(), wa.data() + wa.size(),
                            wb.data(), wb.data() + wb.size()) < 0;
    };

    std::sort(std::execution::par, play_list.begin(), play_list.end(), compare);

    for (auto &item : cache_list)
    {
        LV_LOG_USER("clear cache: %p", item);
        item->clear();
    }
}

play_item* play_list_get_item(uint32_t index)
{
    return play_list[index];
}

uint32_t play_list_get_count()
{
    return play_list.size();
}