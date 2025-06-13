#include "player_mp3.h"
#include "player.h"
#include "stdint.h"
#include "stream.h"
#include "utils.h"

#include "lvgl/src/misc/lv_log.h"

#include "mln_list.h"
#include "mln_utils.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>

#define READ_DIR "/home/coloryr/playlist"

uint32_t time_all = 0;
uint32_t time_now = 0;
uint32_t prerate = 0;

static pthread_t tid;
static sem_t play_sem;
static stream *st;
static mln_list_t play_list = mln_list_null();

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

static void *play_run(void *arg)
{
    for (;;)
    {
        sem_wait(&play_sem);

        music_type type = test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            stream_close(st);
            return NULL;
        }

        stream_seek(st, 0, SEEK_SET);

        switch (type)
        {
        case MUSIC_TYPE_MP3:
            LV_LOG_INFO("Start play mp3");
            mp3_decode_init((stream *)arg);
            mp3_decode_start((stream *)arg);
            mp3_decode_close();
            break;

        default:
            break;
        }
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
            uint32_t len = get_length(full_path);
            play_item *t;
            t = (play_item *)calloc(1, sizeof(*t));
            if (t == NULL)
                continue;
            mln_list_add(&play_list, &t->node);
            t->path = malloc(len);
            t->len = len;
            memcpy(t->path, full_path, len);
        }
    }

    closedir(dp);
}

static void *play_read_run(void *arg)
{
    play_list_close();
    play_read_list(READ_DIR);
    // set view
}

void play_init()
{
    sem_init(&play_sem, 0, 1);

    int res = pthread_create(&tid, NULL, play_run, NULL);
    if (!res)
    {
        LV_LOG_ERROR("Music play thread run fail");
    }

    pthread_t rtid;
    res = pthread_create(&rtid, NULL, play_read_run, NULL);
    if (!res)
    {
        LV_LOG_ERROR("Music play list read thread run fail");
    }
}

void play_file(char *path)
{
    // send stop
    st = stream_open_file(path);
    if (st == NULL)
    {
        return;
    }

    sem_post(&play_sem);
}
