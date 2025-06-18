#include "player.h"

#include "player_mp3.h"
#include "player_flac.h"

#include "mp3_header.h"
#include "stream.h"
#include "utils.h"
#include "mp3id3.h"
#include "sound.h"
#include "view.h"
#include "music_main.h"
#include "music_list.h"

#include "lvgl/src/misc/lv_log.h"

#include "mln_list.h"
#include "mln_utils.h"

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>

#ifdef BUILD_ARM
#define READ_DIR "/sdcard"
#else
#define READ_DIR "/home/coloryr/playlist"
#endif

float time_all = 0;
float time_now = 0;

play_info_item title;
play_info_item album;
play_info_item auther;
play_info_item image;

music_type play_music_type;
mln_list_t play_list = mln_list_null();
uint32_t play_mp3_bps;
uint32_t play_now_index;
uint32_t play_list_count;

static pthread_t tid;
static stream *st;
static music_state play_state;
static music_command play_now_command;
static music_decoder play_decoder;
static uint32_t scan_index = 0;

static music_type test_music_type(stream *st)
{
    uint8_t buffer[4];
    stream_read(st, buffer, 4);

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
    else if (buffer[0] == 0xFF && buffer[1] == 0xE0)
    {
        return MUSIC_TYPE_MP3;
    }

    return MUSIC_TYPE_UNKNOW;
}

static uint8_t *play_get_index(uint32_t index)
{
    play_item *t;
    for (t = mln_container_of(mln_list_head(&play_list), play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), play_item, node))
    {
        if (t->index == index)
        {
            return t->path;
        }
    }

    return NULL;
}

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
    for (;;)
    {
        play_state = MUSIC_STATE_STOP;

        uint8_t *path = play_get_index(play_now_index);

        st = stream_open_file(path);
        if (st == NULL)
        {
            continue;
        }

        play_music_type = test_music_type(st);
        if (play_music_type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            stream_close(st);
            return NULL;
        }

        stream_seek(st, 0, SEEK_SET);
        alsa_reset();

        time_all = 0;
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

        view_fft_clear();

        stream_close(st);
        st = NULL;

        if (play_state != MUSIC_STATE_STOP)
        {
            play_now_command = MUSIC_COMMAND_NEXT;
        }

        play_state = MUSIC_STATE_STOP;

        if (play_now_command == MUSIC_COMMAND_NEXT)
        {
            play_now_index++;
            if (play_now_index >= play_list_count)
            {
                play_now_index = 0;
            }
        }
        else if (play_now_command == MUSIC_COMMAND_LAST)
        {
            if (play_now_index == 0)
            {
                play_now_index = play_list_count - 1;
            }
            else
            {
                play_now_index--;
            }
        }

        play_now_command = MUSIC_COMMAND_UNKNOW;
    }
}

static void play_list_close()
{
    play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&play_list), play_item, node); t != NULL;)
    {
        free(t->path);
        fr = t;
        t = mln_container_of(mln_list_next(&t->node), play_item, node);
        free(fr);
    }
}

static void play_read_list(char *path)
{
    DIR *dp;
    struct dirent *entry;

    dp = opendir(path);
    if (dp == NULL)
    {
        LV_LOG_ERROR("play list opendir fail");
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char full_path[1024] = {0};
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat file_stat;
        if (lstat(full_path, &file_stat) == -1)
        {
            LV_LOG_ERROR("play list read file stat fail");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode))
        {
            play_read_list(full_path); // 递归子目录
        }
        else if (S_ISREG(file_stat.st_mode))
        {
            stream *file = stream_open_file(full_path);
            if (file == NULL)
            {
                return;
            }
            music_type type = test_music_type(file);
            stream_close(file);
            if (type == MUSIC_TYPE_UNKNOW)
            {
                continue;
            }
            uint32_t len = get_length(full_path) + 1;
            play_item *t;
            t = (play_item *)calloc(1, sizeof(*t));
            if (t == NULL)
            {
                continue;
            }
            mln_list_add(&play_list, &t->node);
            t->path = malloc(len);
            t->len = len;
            t->index = scan_index++;
            t->auther = NULL;
            t->title = NULL;
            t->time = 0;
            memcpy(t->path, full_path, len);
            play_list_count++;
        }
    }

    closedir(dp);
}

static void *play_list_info_scan(void *arg)
{
    play_item *t;
    for (t = mln_container_of(mln_list_head(&play_list), play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), play_item, node))
    {
        stream *st = stream_open_file(t->path);
        if (st == NULL)
        {
            continue;
        }

        music_type type = test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            stream_close(st);
            continue;
        }

        stream_seek(st, 0, SEEK_SET);

        switch (type)
        {
        case MUSIC_TYPE_MP3:
            mp3id3 *id3 = mp3_id3_read(st);
            if (id3)
            {
                if (id3->title.data)
                {
                    t->title_len = id3->title.size;
                    t->title = malloc(t->title_len);
                    memcpy(t->title, id3->title.data, t->title_len);
                }
                if (id3->auther.data)
                {
                    t->auther_len = id3->auther.size;
                    t->auther = malloc(t->auther_len);
                    memcpy(t->auther, id3->auther.data, t->auther_len);
                }
                mp3_id3_close(id3);
            }
            else
            {
                stream_seek(st, 0, SEEK_SET);
            }
            t->time = mp3_get_time_len(st);
            break;
        case MUSIC_TYPE_FLAC:
            flac_data *data = flac_read_metadata(st);
            if (data)
            {
                if (data->metadata.title.data)
                {
                    t->title_len = data->metadata.title.size;
                    t->title = malloc(t->title_len);
                    memcpy(t->title, data->metadata.title.data, t->title_len);
                }
                if (data->metadata.auther.data)
                {
                    t->auther_len = data->metadata.auther.size;
                    t->auther = malloc(t->auther_len);
                    memcpy(t->auther, data->metadata.auther.data, t->auther_len);
                }
                t->time = data->metadata.time;
            }
            flac_close_data(data);
            break;
        default:
            break;
        }

        stream_close(st);
    }

    view_update_list();
}

static void *play_read_run(void *arg)
{
    play_list_close();
    scan_index = 0;
    play_read_list(READ_DIR);
    view_init_list();

    play_now_index = 0;

    int res = pthread_create(&tid, NULL, play_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play thread run fail: %d", res);
    }
    pthread_t sid;
    res = pthread_create(&sid, NULL, play_list_info_scan, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list scan thread run fail: %d", res);
    }
}

void play_info_update_flac(flac_metadata *data)
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
    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_read_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
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
    default:
        break;
    }

    return false;
}