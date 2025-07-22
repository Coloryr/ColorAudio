#include "local_music.h"

#include "music.h"
#include "mp3_id3.h"
#include "mp3_header.h"
#include "flac_metadata.h"
#include "dep.h"

#include "../player/player.h"
#include "../stream/stream_file.h"
#include "../net/music_api.h"
#include "../ui/ui.h"
#include "../ui/music_view.h"
#include "../ui/view_state.h"
#include "../ui/info_view.h"
#include "../config/config.h"
#include "../common/utilspp.h"

#include "../lvgl/src/misc/lv_log.h"

#include <unistd.h>
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
#include <json/json.hpp>

using namespace ColorAudio;

static void play_list_close()
{
    for (auto it = play_list.begin(); it != play_list.end(); ++it)
    {
        delete it->second;
    }

    play_list.clear();
    play_list_count = 0;
}

static void play_read_list(const char *path)
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
            StreamFile st = StreamFile(full_path);
            music_type type = play_test_music_type(&st);
            if (type == MUSIC_TYPE_UNKNOW)
            {
                continue;
            }
            play_item *t = new play_item();
            t->path = full_path;
            t->index = play_list_count++;
            t->auther = "";
            t->title = "";
            t->time = 0;

            play_list[t->index] = t;
        }
    }

    closedir(dp);
}

static void *play_list_info_scan(void *arg)
{
    play_item *t;
    for (const auto &it : play_list)
    {
        t = it.second;
        StreamFile st = StreamFile(t->path);
        music_type type = play_test_music_type(&st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            continue;
        }

        switch (type)
        {
        case MUSIC_TYPE_MP3:
        {
            Mp3Id3 id3 = Mp3Id3(&st);
            if (id3.get_info())
            {
                t->title = id3.title;
                t->auther = id3.auther;
            }
            else
            {
                st.seek(0, SEEK_SET);
            }
            t->time = mp3_get_time_len(&st);
            break;
        }
        case MUSIC_TYPE_FLAC:
        {
            FlacMetadata data = FlacMetadata(&st);
            if (data.decode_get_info())
            {
                t->title = data.info.title;
                t->auther = data.info.auther;
                t->time = data.info.time;
            }
            break;
        }
        }
    }

    view_update_list();

    return NULL;
}

static void get_music_lyric(std::string &comment)
{
    try
    {
        if (comment.find("163 key(Don't modify):") != 0)
        {
            view_music_set_lyric_state(LYRIC_NONE);
            return;
        }
        std::string key = comment.substr(22);
        std::string temp = dep(key);
        if (temp.empty())
        {
            view_music_set_lyric_state(LYRIC_NONE);
            return;
        }
        temp = temp.substr(6);
        json j1 = json::parse(temp);
        json id = j1["musicId"];
        if (id.is_string())
        {
            uint64_t id1 = std::stoul(id.get<std::string>());

            music_lyric_163(id1);
        }
    }
    catch (const std::exception &e)
    {
        LV_LOG_ERROR("%s", e.what());
        view_music_set_lyric_state(LYRIC_FAIL);
    }
}

static void local_music_run()
{
    for (;;)
    {
        music_test_run(MUSIC_RUN_LOCAL);

        pthread_mutex_lock(&play_mutex);
        play_item *item = play_list[play_now_index];

        StreamFile st = StreamFile(item->path);
        music_type type = play_test_music_type(&st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            continue;
        }

        music_start();

        if (type == MUSIC_TYPE_MP3)
        {
            Mp3Id3 id3 = Mp3Id3(&st);
            if (id3.get_info())
            {
                comment = id3.comment;

                play_update_text(id3.title, MUSIC_INFO_TITLE);
                play_update_text(id3.auther, MUSIC_INFO_AUTHER);
                play_update_text(id3.album, MUSIC_INFO_ALBUM);
                play_update_image(id3.image->copy(), MUSIC_INFO_IMAGE);

                view_update_img();
            }

            play_st = st.copy();
            pthread_cond_signal(&play_start);
            pthread_mutex_unlock(&play_mutex);

            time_all = 0;
            float scan_time = mp3_get_time_len(&st);

            if (scan_time == 0)
            {
                LV_LOG_USER("time is 0");
            }

            time_all = scan_time;

            view_update_info();
        }
        else if (type == MUSIC_TYPE_FLAC)
        {
            play_st = st.copy();
            pthread_cond_signal(&play_start);
            pthread_mutex_unlock(&play_mutex);

            FlacMetadata flac = FlacMetadata(&st);
            if (flac.decode_get_info())
            {
                comment = flac.info.comment;

                play_update_text(flac.info.title, MUSIC_INFO_TITLE);
                play_update_text(flac.info.auther, MUSIC_INFO_AUTHER);
                play_update_text(flac.info.album, MUSIC_INFO_ALBUM);
                play_update_image(flac.info.image->copy(), MUSIC_INFO_IMAGE);

                time_all = flac.info.time;

                view_update_info();
                view_update_img();
            }
        }

        if (!comment.empty())
        {
            get_music_lyric(comment);
        }
        else
        {
            view_music_set_lyric_state(LYRIC_NONE);
        }

        usleep(1000);

        std::string name;
        getfilename(item->path, name);
        config::set_config_music_name(name);
        config::set_config_music_index(play_now_index);
        config::save_config();

        // 等待播放结束
        pthread_mutex_lock(&play_mutex);

        music_end();

        pthread_mutex_unlock(&play_mutex);
    }
}

static void *play_read_run(void *arg)
{
    play_list_close();
    play_read_list(READ_DIR);
    view_init_list();

    if (play_list.empty())
    {
        return NULL;
    }

    std::string name = config::get_config_music_name();
    uint32_t index = config::get_config_music_index();

    play_now_index = 0;

    if (play_list.contains(index) && !name.empty())
    {
        play_item *item = play_list[index];
        if (endsWith(item->path, name))
        {
            play_now_index = index;
        }
        else
        {
            for (const auto &item : play_list)
            {
                if (endsWith(item.second->path, name))
                {
                    play_now_index = item.first;
                    break;
                }
            }
        }
    }

    pthread_t sid;
    int res = pthread_create(&sid, NULL, play_list_info_scan, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list scan thread run fail: %d", res);
    }

    return NULL;
}

void local_music_init()
{
    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_read_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
}