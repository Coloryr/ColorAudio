#include "player.h"

#include "decoder/decoder.h"
#include "decoder/decoder_flac.h"
#include "decoder/decoder_mp3.h"
#include "sound.h"

#include "../music/mp3_id3.h"
#include "../music/music.h"
#include "../stream/stream.h"
#include "../ui/ui.h"
#include "../ui/view_state.h"
#include "../config/config.h"

#include "../lvgl/src/misc/lv_log.h"

#include <boost/container/flat_map.hpp>
#include <stdint.h>
#include <pthread.h>
#include <string>
#include <map>

using namespace ColorAudio;

static pthread_t tid;

static music_command play_now_command = MUSIC_COMMAND_UNKNOW;

static float jump_time = 0;

boost::container::flat_map<uint32_t, play_item *> play_list;

std::string title;
std::string album;
std::string auther;
std::string comment;
data_item *image;

ColorAudio::Stream *play_st;

pthread_mutex_t play_mutex;
pthread_cond_t play_start;

music_type play_test_music_type(ColorAudio::Stream *st)
{
    uint8_t buffer[4];
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

    return MUSIC_TYPE_UNKNOW;
}

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

        play_music_type = play_test_music_type(play_st);
        if (play_music_type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            delete play_st;
            continue;
        }

        alsa_reset();

        time_now = 0;

        play_state = MUSIC_STATE_PLAY;

        view_update_state();

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
            view_update_state();
            LV_LOG_USER("play decoder run fail");
        }
        delete play_decoder;

        if (target_time > 0 && play_now_command == MUSIC_COMMAND_UNKNOW)
        {
            alsa_clear();
            play_st->seek(start_pos, SEEK_SET);
            play_state = MUSIC_STATE_SEEK;
            goto play;
        }

        target_time = 0;

        view_music_clear();

        delete play_st;

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
        title = text;
        break;
    case MUSIC_INFO_AUTHER:
        auther = text;
        break;
    case MUSIC_INFO_ALBUM:
        album = text;
        break;
    default:
        break;
    }
}

void play_clear()
{
    title.clear();
    album.clear();
    auther.clear();
    comment.clear();

    play_update_image(nullptr, MUSIC_INFO_IMAGE);

    view_update_info();
    view_update_img();
}

void play_update_image(data_item *data, music_info_type type)
{
    switch (type)
    {
    case MUSIC_INFO_IMAGE:
        if (image)
        {
            delete image;
        }
        image = data;
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
        play_music_mode = static_cast<music_mode>((play_music_mode + 1) % 2);
        config::set_config_music_code(play_music_mode);
        config::save_config();
        view_update_state();
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
    target_time = time / 1000;
    jump_time = target_time;
    play_state = MUSIC_STATE_STOP;
}

void play_jump_end()
{
    play_state = MUSIC_STATE_PLAY;
    time_now = jump_time - target_time;
    jump_time = 0;
    target_time = 0;
}

music_command play_get_command()
{
    return play_now_command;
}