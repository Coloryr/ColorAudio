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

#include <boost/container/vector.hpp>
#include <json/json.hpp>
#include "lvgl/src/misc/lv_log.h"
#include "ncmcrypt.h"

#include "music.h"
#include "music_player.h"
#include "mp3/mp3_id3.h"
#include "mp3/mp3_header.h"
#include "flac/flac_metadata.h"
#include "common/timestamp.h"
#include "common/utilspp.h"
#include "stream/stream_file.h"
#include "stream/stream_ncm.h"
#include "net/music_api.h"
#include "ui/music_view.h"
#include "ui/info_view.h"
#include "config/config.h"

#include "local_music.h"

using namespace coloraudio::config;
using namespace coloraudio::stream;
using namespace coloraudio::common;
using namespace coloraudio::mp3;
using namespace coloraudio::flac;

bool local_music_scan_now;

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
            FileStream st = FileStream(full_path);
            music_type type = music_test_type(&st);
            if (type == MUSIC_TYPE_UNKNOW)
            {
                continue;
            }
            std::string path = full_path;
            std::string title;
            std::string auther;
            float time;
            if (type == MUSIC_TYPE_MP3)
            {
                Mp3Id3 id3 = Mp3Id3(&st);
                if (id3.get_info())
                {
                    title = id3.title;
                    auther = id3.auther;
                }
                else
                {
                    title = "...";
                    auther = "...";
                    st.seek(0, SEEK_SET);
                }
                time = mp3_get_time_len(&st);
            }
            else if (type == MUSIC_TYPE_FLAC)
            {
                FlacMetadata data = FlacMetadata(&st);
                if (data.decode_get_info())
                {
                    title = data.info.title;
                    auther = data.info.auther;
                    time = data.info.time;
                }
                else
                {
                    title = "...";
                    auther = "...";
                    time = 0;
                }
            }
            else if (type == MUSIC_TYPE_NCM)
            {
                NeteaseCrypt cry = NeteaseCrypt(&st, true);
                if (cry.mMetaData != nullptr)
                {
                    title = cry.mMetaData->name();
                    auther = cry.mMetaData->artist();
                    time = (float)cry.mMetaData->duration() / 1000;
                }
                else
                {
                    title = "...";
                    auther = "...";
                    time = 0;
                }
            }

            play_list_add_item(path, title, auther, time);
        }
    }

    closedir(dp);
}

static void *play_scan_run(void *arg)
{
    uint32_t time = clock_ms();
    uint32_t time1;
    local_music_scan_now = true;
    play_list_close();
    play_read_list(READ_DIR);

    time1 = clock_ms();
    LV_LOG_USER("read list time: %d", time1 - time);
    time = clock_ms();

    if (play_list_empty())
    {
        local_music_scan_now = false;
        return NULL;
    }

    play_list_sort_by_pinyin();

    time1 = clock_ms();
    LV_LOG_USER("sort time: %d", time1 - time);

    play_list_read_done();

    view_music_init_list();
    local_music_scan_now = false;
    return NULL;
}

void local_music_init()
{
    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_scan_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
        return;
    }
    pthread_setname_np(rtid, "playlist_scan");
    pthread_detach(rtid);
}

void local_music_run(play_item *item)
{
    FileStream st = FileStream(item->path);
    music_type type = music_test_type(&st);
    if (type == MUSIC_TYPE_UNKNOW)
    {
        LV_LOG_ERROR("Unkown music file type");
        return;
    }

    music_start();

    std::string comment;

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

            view_music_update_img();
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

        view_music_update_info();
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

            view_music_update_info();
            view_music_update_img();
        }
    }
    else if (type == MUSIC_TYPE_NCM)
    {
        NeteaseCrypt *cry = new NeteaseCrypt(&st, true);
        play_st = new NcmStream(st.copy(), cry);

        pthread_cond_signal(&play_start);
        pthread_mutex_unlock(&play_mutex);

        if (cry->mMetaData != nullptr)
        {
            comment = cry->modify;

            play_update_text(cry->mMetaData->name(), MUSIC_INFO_TITLE);
            play_update_text(cry->mMetaData->artist(), MUSIC_INFO_AUTHER);
            play_update_text(cry->mMetaData->album(), MUSIC_INFO_ALBUM);
            if (cry->mImageData)
            {
                play_update_image(new DataItem(cry->mImageData, cry->imageSize), MUSIC_INFO_IMAGE);
            }

            time_all = cry->mMetaData->duration() / 1000;

            view_music_update_info();
            view_music_update_img();
        }
    }

    if (!comment.empty())
    {
        music_get_lyric(comment);
    }
    else
    {
        view_music_set_lyric_state(LYRIC_NONE);
    }
}