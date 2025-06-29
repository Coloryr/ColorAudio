#include "local_music.h"

#include "utils.h"
#include "player.h"
#include "mp3_header.h"
#include "music_main.h"
#include "music_list.h"
#include "view.h"
#include "mp3id3.h"
#include "player_flac.h"
#include "music.h"

#include "lvgl/src/misc/lv_log.h"
#include "mln_list.h"
#include "mln_utils.h"
#include "mln_stack.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>

#ifdef BUILD_ARM
#define READ_DIR "/sdcard"
#else
#define READ_DIR "/home/coloryr/playlist"
// #define READ_DIR "/home/coloryr/playtest"
#endif

static uint32_t scan_index = 0;
static mln_stack_t *play_last_stack;

mln_list_t local_play_list = mln_list_null();

uint32_t play_now_index;
uint32_t play_list_count;

music_mode play_music_mode = MUSIC_MODE_LOOP;

static uint8_t *play_get_index(uint32_t index)
{
    play_item *t;
    for (t = mln_container_of(mln_list_head(&local_play_list), play_item, node);
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

static void play_list_close()
{
    play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&local_play_list), play_item, node); t != NULL;)
    {
        free(t->path);
        fr = t;
        t = mln_container_of(mln_list_next(&t->node), play_item, node);
        free(fr);
    }
}

static void *copy(uint32_t *d, void *data)
{
    uint32_t *dup;
    assert((dup = (uint32_t *)malloc(sizeof(uint32_t))) != NULL);
    *dup = *d;
    return dup;
}

static int play_last_stack_is(void *data, void *value)
{
    uint32_t *temp = data;
    uint32_t *temp1 = value;
    if (*temp == *temp1)
    {
        return -1;
    }
    return 0;
}

static int play_last_stack_count_is(void *data, void *value)
{
    uint32_t *temp1 = value;
    *temp1++;
    return 0;
}

static uint32_t play_last_stack_count()
{
    uint32_t count = 0;
    mln_stack_iterate(play_last_stack, play_last_stack_count_is, &count);
    return count;
}

static uint32_t play_last_stack_get()
{
    uint32_t *d;
    d = mln_stack_pop(play_last_stack);
    if (d != NULL)
    {
        uint32_t temp = *d;
        free(d);
        return temp;
    }

    return UINT32_MAX;
}

static void play_last_stack_push(uint32_t index)
{
    uint32_t *d = (uint32_t *)malloc(sizeof(uint32_t));
    *d = index;
    mln_stack_push(play_last_stack, d);
}

static void play_last_stack_old()
{
    if (play_last_stack_count() > play_list_count / 10)
    {
        uint32_t *old = mln_stack_pop(play_last_stack);
        free(old);
    }
}

static uint32_t read_random()
{
    uint32_t temp;
    int fd = open("/dev/random", O_RDONLY);
    read(fd, &temp, 4);
    close(fd);

    return temp;
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
            stream_t *file = stream_open_file(full_path);
            if (file == NULL)
            {
                return;
            }
            music_type type = play_test_music_type(file);
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
            mln_list_add(&local_play_list, &t->node);
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
    for (t = mln_container_of(mln_list_head(&local_play_list), play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), play_item, node))
    {
        stream_t *st = stream_open_file(t->path);
        if (st == NULL)
        {
            continue;
        }

        music_type type = play_test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            stream_close(st);
            continue;
        }

        switch (type)
        {
        case MUSIC_TYPE_MP3:
            mp3id3 *id3 = mp3id3_read(st);
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
                mp3id3_close(id3);
            }
            else
            {
                stream_seek(st, 0, SEEK_SET);
            }
            t->time = mp3_get_time_len(st);
            break;
        case MUSIC_TYPE_FLAC:
            flac_data_t *data = flac_read_metadata(st);
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
            flac_data_close(data);
            break;
        default:
            break;
        }

        stream_close(st);
    }

    view_update_list();
}

static void local_music_run()
{
    for (;;)
    {
        music_test_run(MUSIC_RUN_LOCAL);

        pthread_mutex_lock(&play_mutex);
        uint8_t *path = play_get_index(play_now_index);

        stream_t *st = stream_open_file(path);
        if (st == NULL)
        {
            continue;
        }

        music_type type = play_test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            stream_close(st);
            continue;
        }

        if (type == MUSIC_TYPE_MP3)
        {
            mp3id3 *id3 = mp3id3_read(st);
            if (id3)
            {
                play_info_update_id3(id3);
                mp3id3_close(id3);
            }

            stream_t *st1 = stream_copy_file(st);
            play_st = st1;
            pthread_cond_signal(&play_start);
            pthread_mutex_unlock(&play_mutex);

            time_all = 0;
            float scan_time = mp3_get_time_len(st);

            if (scan_time == 0)
            {
                LV_LOG_USER("time is 0");
            }

            time_all = scan_time;
        }
        else if (type == MUSIC_TYPE_FLAC)
        {
            stream_t *st1 = stream_copy_file(st);
            play_st = st1;
            pthread_cond_signal(&play_start);
            pthread_mutex_unlock(&play_mutex);

            flac_data_t *flac = flac_read_metadata(st);
            if (flac != NULL)
            {
                play_info_update_flac(&flac->metadata);

                flac_data_close(flac);

                stream_seek(st, 0, SEEK_SET);
            }
        }

        usleep(1000);

        // 等待播放结束
        pthread_mutex_lock(&play_mutex);

        // 关闭
        stream_close(st);

        if (get_play_command() == MUSIC_COMMAND_NEXT)
        {
            if (play_music_mode == MUSIC_MODE_RND)
            {
                play_last_stack_push(play_now_index);
                uint32_t next_value;
                do
                {
                    next_value = read_random() % play_list_count;
                } while (mln_stack_iterate(play_last_stack, play_last_stack_is, &next_value) == -1);
                play_last_stack_old();
                play_now_index = next_value;
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
        else if (get_play_command() == MUSIC_COMMAND_LAST)
        {
            if (play_music_mode == MUSIC_MODE_RND)
            {
                if (mln_stack_empty(play_last_stack))
                {
                    goto last_go;
                }
                else
                {
                    uint32_t *old = mln_stack_pop(play_last_stack);
                    play_now_index = *old;
                    free(old);
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

        // 清理指令
        play_command(MUSIC_COMMAND_UNKNOW);
        pthread_mutex_unlock(&play_mutex);
    }
}

static void *play_read_run(void *arg)
{
    play_list_close();
    scan_index = 0;
    play_read_list(READ_DIR);
    view_init_list();

    play_now_index = 0;

    pthread_t sid;
    int res = pthread_create(&sid, NULL, play_list_info_scan, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list scan thread run fail: %d", res);
    }

    local_music_run();
}

void local_music_init()
{
    play_last_stack = mln_stack_init((stack_free)free, (stack_copy)copy);

    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_read_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
}