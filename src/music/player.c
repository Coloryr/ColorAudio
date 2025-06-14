#include "player.h"

#include "player_mp3.h"
#include "play_flac.h"

#include "stream.h"
#include "utils.h"
#include "mp3id3.h"
#include "view.h"
#include "fft/FFT.h"
#include "music_main.h"
#include "sound.h"

#include "lvgl/src/misc/lv_log.h"

#include "mln_list.h"
#include "mln_utils.h"

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#define READ_DIR "/home/coloryr/playlist"

float time_all = 0;
float time_now = 0;

play_info_item title;
play_info_item album;
play_info_item auther;
play_info_item image;

bool is_play = false;
bool is_pause = false;
bool is_stop = false;

int32_t *buf;
int32_t *buf1;
uint32_t last_pcm_size;
uint8_t last_pcm_bps;

static pthread_t tid;
static sem_t play_sem;
static stream *st;
static mln_list_t play_list = mln_list_null();
static uint32_t play_list_count;
static uint32_t now_index;
static bool go_next = false;
static bool go_last = false;
static music_state play_state;

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
        play_state = MUSIC_STATE_STOP;
        sem_wait(&play_sem);

        music_type type = test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            stream_close(st);
            return NULL;
        }

        stream_seek(st, 0, SEEK_SET);
        alsa_reset();

        time_all = 0;
        time_now = 0;

        is_stop = false;

        play_state = MUSIC_STATE_PLAY;

        switch (type)
        {
        case MUSIC_TYPE_MP3:
            LV_LOG_USER("Start play mp3");
            mp3_decode_init((stream *)st);
            mp3_decode_start((stream *)st);
            mp3_decode_close();
            break;
        case MUSIC_TYPE_FLAC:
            LV_LOG_USER("Start play flac");
            flac_decode_init((stream *)st);
            flac_decode_start((stream *)st);
            flac_decode_close();
            break;
        default:
            break;
        }

        stream_close(st);

        if (!is_stop)
        {
            go_next = true;
        }

        play_state = MUSIC_STATE_STOP;

        if (go_next)
        {
            now_index++;
            if (now_index >= play_list_count)
            {
                now_index = 0;
            }
            play_index(now_index);
            go_next = false;
        }
        if (go_last)
        {
            if (now_index == 0)
            {
                now_index = play_list_count - 1;
            }
            play_index(now_index);
            go_next = false;
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
            memcpy(t->path, full_path, len);
            play_list_count++;
        }
    }

    closedir(dp);
}

static void *play_read_run(void *arg)
{
    play_list_close();
    play_read_list(READ_DIR);
    // set view

    play_index(0);
}

void play_set_state(bool isplay)
{
    is_play = isplay;

    view_update_state();
}

void play_info_update_image(uint8_t *data, uint32_t size)
{
    if (image.data)
    {
        free(image.data);
        image.data = NULL;
    }
    if (data)
    {
        image.data = malloc(size);
        image.size = size;
        memcpy(image.data, data, size);
        view_update_img();
    }
}

void play_info_update_title(uint8_t *data, uint32_t size)
{
    if (title.data)
    {
        free(title.data);
        title.data = NULL;
    }
    if (data)
    {
        title.data = malloc(size);
        title.size = size;
        memcpy(title.data, data, size);
        view_update_info();
    }
}

void play_info_update_album(uint8_t *data, uint32_t size)
{
    if (album.data)
    {
        free(album.data);
        album.data = NULL;
    }
    if (data)
    {
        album.data = malloc(size);
        album.size = size;
        memcpy(album.data, data, size);
        view_update_info();
    }
}

void play_info_update_auther(uint8_t *data, uint32_t size)
{
    if (auther.data)
    {
        free(auther.data);
        auther.data = NULL;
    }
    if (data)
    {
        auther.data = malloc(size);
        auther.size = size;
        memcpy(auther.data, data, size);
        view_update_info();
    }
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

static uint16_t input_fft_index;
#define POINTS 2048
#define OUT_POINTS 20
#define SQRT2 1.4142135623730951

static float input_fft_data[POINTS];
static float input_fft_data_imag[POINTS];
static float freqs[POINTS / 2];
static int freq_bands[] = {50, 69, 94, 129, 176, 241, 331, 453, 620, 850, 1200, 1600, 2200, 3000, 4100, 5600, 7700, 11000, 14000, 20000};

void fill_fft(uint16_t rate, uint16_t size, int32_t data[], uint32_t down)
{
    bool skip = false;
    if (size >= POINTS)
    {
        for (size_t i = 0; i < POINTS; i++)
        {
            input_fft_data[i] = ((float)data[i]) / down * POINTS / (POINTS / 2) / SQRT2;
            input_fft_data_imag[i] = input_fft_data[i];
        }
    }
    else
    {
        uint16_t less = POINTS - input_fft_index;
        if (input_fft_index + size < POINTS)
        {
            skip = true;
        }
        for (size_t i = 0; i < LV_MIN(less, size); i++)
        {
            input_fft_data[input_fft_index] = ((float)data[i]) / down * POINTS / (POINTS / 2) / SQRT2;
            input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index];
            input_fft_index++;
        }
    }
    if (skip)
    {
        return;
    }

    input_fft_index = 0;

    fft_run(input_fft_data, input_fft_data_imag, POINTS);

    uint16_t len = POINTS / 2;

    for (uint16_t i = 0; i < len; i++)
    {
        float freq = (float)i * rate / POINTS;
        freqs[i] = freq;
    }

    uint16_t index = 0;

    float bin_values[OUT_POINTS] = {0};
    int j = 0;
    for (int bin = 0; bin < OUT_POINTS; bin++)
    {
        int start_idx = j;
        for (; j < len; j++)
        {
            if (freqs[j] > freq_bands[bin])
            {
                break;
            }
        }

        int end_idx = j;

        for (int j = start_idx; j < end_idx; j++)
        {
            bin_values[bin] = LV_MAX(input_fft_data[j], bin_values[bin]);
            bin_values[bin] += input_fft_data[j];
        }
        bin_values[bin] /= (end_idx - start_idx);
        bin_values[bin] = log10f(bin_values[bin]);
        if (bin_values[bin] < 0)
        {
            bin_values[bin] = 0;
        }
    }

    for (int i = 0; i < OUT_POINTS; i++)
    {
        int bar_len = (int)(bin_values[i] * 20);
        view_set_fft_data(i, bar_len, 0);
    }
}

void play_init()
{
    sem_init(&play_sem, 0, 0);

    int res = pthread_create(&tid, NULL, play_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play thread run fail: %d", res);
    }

    pthread_t rtid;
    res = pthread_create(&rtid, NULL, play_read_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
}

void play_file(char *path)
{
    st = stream_open_file(path);
    if (st == NULL)
    {
        return;
    }

    sem_post(&play_sem);
}

void play_index(uint32_t index)
{
    now_index = index;
    uint32_t i = 0;
    play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&play_list), play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), play_item, node))
    {
        if (i == index)
        {
            play_file(t->path);
            break;
        }
        else
        {
            i++;
        }
    }
}

bool play_resume()
{
    if (play_state == MUSIC_STATE_PAUSE)
    {
        is_pause = false;
        play_state = MUSIC_STATE_PLAY;
        return true;
    }
}

bool play_pause()
{
    if (play_state == MUSIC_STATE_PLAY)
    {
        is_pause = true;
        play_state = MUSIC_STATE_PAUSE;
        return false;
    }
}

void play_stop()
{
    is_stop = true;
}

void play_next()
{
    play_stop();
    go_next = true;
}

void play_last()
{
    play_stop();
    go_last = true;
}