#include "player.h"
#include "stdint.h"
#include "stream.h"

#include "lvgl/src/misc/lv_log.h"

#include "player_mp3.h"

#include <pthread.h>
#include <semaphore.h>

uint32_t time_all = 0;
uint32_t time_now = 0;
uint32_t prerate = 0;

static pthread_t tid;
static sem_t play_sem;
static stream *st;

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
            return;
        }

        stream_seek(st, 0, SEEK_SET);

        switch (type)
        {
        case MUSIC_TYPE_MP3:
            LV_LOG_INFO("Start play mp3");
            mp3_decode((stream *)arg);
            break;

        default:
            break;
        }
    }
}

void play_read_list()
{
    
}

void play_init()
{
    sem_init(&play_sem, 0, 1);

    int res = pthread_create(&tid, NULL, play_run, NULL);
    if (!res)
    {
        LV_LOG_ERROR("Music play thread run fail");
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
