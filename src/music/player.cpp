#include "player.h"

#include "player_mp3.h"
#include "player_flac.h"
#include "mp3_header.h"
#include "stream.h"
#include "utils.h"
#include "mp3id3.h"
#include "sound.h"
#include "music_main.h"

#include "lvgl/src/misc/lv_log.h"

#include <stdint.h>
#include <pthread.h>

#include <string>

using namespace std;

static pthread_t tid;
static music_state play_state;
static music_command play_now_command = MUSIC_COMMAND_UNKNOW;
static music_decoder play_decoder;

static float jump_time = 0;

float target_time = 0;

float time_all = 0;
float time_now = 0;

string title;
string album;
string auther;
string image;

Stream *play_st;

music_type play_music_type;

uint32_t play_mp3_bps;

pthread_mutex_t play_mutex;
pthread_cond_t play_start;

static void play_use_mp3_decoder()
{
    play_decoder.decode_init = mp3_decode_init;
    play_decoder.decode_start = mp3_decode_start;
    play_decoder.decode_close = mp3_decode_close;
}

static void play_use_flac_decoder()
{
    play_decoder.decode_init = flac_decode_init;
    play_decoder.decode_start = flac_decode_start;
    play_decoder.decode_close = flac_decode_close;
}

static void *play_run(void *arg)
{
    stream_t *st;

    for (;;)
    {
        play_state = MUSIC_STATE_STOP;

        pthread_mutex_lock(&play_mutex);
        while (play_st == NULL)
            pthread_cond_wait(&play_start, &play_mutex);
        st = play_st;
        play_st = NULL;

        play_music_type = play_test_music_type(st);
        if (play_music_type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            stream_close(st);
            return NULL;
        }

        alsa_reset();

        time_now = 0;

        play_state = MUSIC_STATE_PLAY;

        view_update_state();
        view_update_list_index();

        switch (play_music_type)
        {
        case MUSIC_TYPE_MP3:
            LV_LOG_USER("Start play mp3");
            play_use_mp3_decoder();
            break;
        case MUSIC_TYPE_FLAC:
            LV_LOG_USER("Start play flac");
            play_use_flac_decoder();
            break;
        default:
            break;
        }

        uint32_t start_pos = stream_get_pos(st);

    play:
        if (!play_decoder.decode_init(st))
        {
            LV_LOG_USER("play decoder init fail");
        }
        else if (!play_decoder.decode_start(st))
        {
            view_update_state();
            LV_LOG_USER("play decoder run fail");
        }
        play_decoder.decode_close();

        if (target_time > 0 && play_now_command == MUSIC_COMMAND_UNKNOW)
        {
            alsa_clear();
            stream_seek(st, start_pos, SEEK_SET);
            play_state = MUSIC_STATE_SEEK;
            goto play;
        }

        target_time = 0;

        lv_music_fft_clear();

        st = NULL;

        // 自动下一首
        if (play_state != MUSIC_STATE_STOP)
        {
            play_now_command = MUSIC_COMMAND_NEXT;
        }
        play_state = MUSIC_STATE_STOP;

        pthread_mutex_unlock(&play_mutex);
    }
}

music_type play_test_music_type(stream_t *st)
{
    uint8_t buffer[4];
    stream_peek(st, buffer, 4);

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

void play_info_update_raw(uint8_t *data, uint32_t size, music_info_type type)
{
    switch (type)
    {
    case MUSIC_INFO_TITLE:
        if (title.data)
        {
            free(title.data);
            title.data = NULL;
        }
        title.data = malloc(size);
        title.size = size;
        memcpy(title.data, data, size);
        break;
    case MUSIC_INFO_AUTHER:
        if (auther.data)
        {
            free(auther.data);
            auther.data = NULL;
        }
        auther.data = malloc(size);
        auther.size = size;
        memcpy(auther.data, data, size);
        break;
    case MUSIC_INFO_ALBUM:
        if (album.data)
        {
            free(album.data);
            album.data = NULL;
        }
        album.data = malloc(size);
        album.size = size;
        memcpy(album.data, data, size);
        break;
    case MUSIC_INFO_IMAGE:
        if (image.data)
        {
            free(image.data);
            image.data = NULL;
        }
        image.data = malloc(size);
        image.size = size;
        memcpy(image.data, data, size);
        break;
    default:
        break;
    }
}

void play_info_update_flac(flac_metadata_t *data)
{
    time_all = data->time;

    if (title.data)
    {
        free(title.data);
        title.data = NULL;
    }
    if (album.data)
    {
        free(album.data);
        album.data = NULL;
    }
    if (auther.data)
    {
        free(auther.data);
        auther.data = NULL;
    }
    if (image.data)
    {
        free(image.data);
        image.data = NULL;
    }

    if (data->title.data)
    {
        title.data = malloc(data->title.size);
        title.size = data->title.size;
        memcpy(title.data, data->title.data, data->title.size);
    }
    if (data->album.data)
    {
        album.data = malloc(data->album.size);
        album.size = data->album.size;
        memcpy(album.data, data->album.data, data->album.size);
    }
    if (data->auther.data)
    {
        auther.data = malloc(data->auther.size);
        auther.size = data->auther.size;
        memcpy(auther.data, data->auther.data, data->auther.size);
    }
    if (data->image.data)
    {
        image.data = malloc(data->image.size);
        image.size = data->image.size;
        memcpy(image.data, data->image.data, data->image.size);
    }

    view_update_img();
    view_update_info();
}

void play_info_update_id3(mp3id3 *id3)
{
    if (title.data)
    {
        free(title.data);
        title.data = NULL;
    }
    if (album.data)
    {
        free(album.data);
        album.data = NULL;
    }
    if (auther.data)
    {
        free(auther.data);
        auther.data = NULL;
    }
    if (image.data)
    {
        free(image.data);
        image.data = NULL;
    }

    if (id3->title.data)
    {
        title.data = malloc(id3->title.size);
        title.size = id3->title.size;
        memcpy(title.data, id3->title.data, id3->title.size);
    }
    if (id3->album.data)
    {
        album.data = malloc(id3->album.size);
        album.size = id3->album.size;
        memcpy(album.data, id3->album.data, id3->album.size);
    }
    if (id3->auther.data)
    {
        auther.data = malloc(id3->auther.size);
        auther.size = id3->auther.size;
        memcpy(auther.data, id3->auther.data, id3->auther.size);
    }
    if (id3->image.data)
    {
        image.data = malloc(id3->image.size);
        image.size = id3->image.size;
        memcpy(image.data, id3->image.data, id3->image.size);
    }

    view_update_img();
    view_update_info();
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

music_state get_play_state()
{
    return play_state;
}

void play_jump_index(uint32_t index)
{
    if (index >= play_list_count)
    {
        play_now_index = play_list_count - 1;
    }
    else
    {
        play_now_index = index;
    }

    play_state = MUSIC_STATE_STOP;
}

bool play_command(music_command command)
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
        play_music_mode = (play_music_mode + 1) % 2;
        view_update_state();
        return true;
    case MUSIC_COMMAND_UNKNOW:
        play_now_command = MUSIC_COMMAND_UNKNOW;
        return true;
    default:
        return false;
    }
}

void play_set_volume(uint16_t value)
{
    alsa_set_volume(value);
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