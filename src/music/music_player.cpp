#include <stdint.h>
#include <pthread.h>
#include <string>
#include <map>

#include "lvgl/src/misc/lv_log.h"
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

#include "music_player.h"

using namespace coloraudio::common;
using namespace coloraudio::decoder;
using namespace coloraudio::stream;
using namespace coloraudio::config;

static pthread_t tid;

static music_command play_now_command = MUSIC_COMMAND_UNKNOW;

static float jump_time = 0;

boost::container::vector<play_item *> play_list;

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