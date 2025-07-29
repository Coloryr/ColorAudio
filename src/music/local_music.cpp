#include "local_music.h"

#include "music.h"
#include "music_player.h"
#include "mp3/mp3_id3.h"
#include "mp3/mp3_header.h"
#include "flac/flac_metadata.h"
#include "163/music163.h"

#include "../common/timestamp.h"
#include "../stream/stream_file.h"
#include "../stream/stream_ncm.h"
#include "../net/music_api.h"
#include "../ui/music_view.h"
#include "../ui/info_view.h"
#include "../config/config.h"
#include "../common/utilspp.h"

#include "../lvgl/src/misc/lv_log.h"
#include "ncmcrypt.h"

#include <boost/container/vector.hpp>
#include <boost/locale.hpp>
#include <unistd.h>
#include <stdbool.h>
#include <execution>
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

bool local_music_scan_now;

static pthread_t rtid;

static void play_list_close()
{
    for (const auto &it : play_list)
    {
        delete it;
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

            if (type == MUSIC_TYPE_MP3)
            {
                Mp3Id3 id3 = Mp3Id3(&st);
                if (id3.get_info())
                {
                    t->title = id3.title;
                    t->auther = id3.auther;
                }
                else
                {
                    t->title = "...";
                    t->auther = "...";
                    st.seek(0, SEEK_SET);
                }
                t->time = mp3_get_time_len(&st);
            }
            else if (type == MUSIC_TYPE_FLAC)
            {
                FlacMetadata data = FlacMetadata(&st);
                if (data.decode_get_info())
                {
                    t->title = data.info.title;
                    t->auther = data.info.auther;
                    t->time = data.info.time;
                }
                else
                {
                    t->title = "...";
                    t->auther = "...";
                    t->time = 0;
                }
            }
            else if (type == MUSIC_TYPE_NCM)
            {
                NeteaseCrypt cry = NeteaseCrypt(&st, true);
                if (cry.mMetaData != nullptr)
                {
                    t->title = cry.mMetaData->name();
                    t->auther = cry.mMetaData->artist();
                    t->time = (float)cry.mMetaData->duration() / 1000;
                }
                else
                {
                    t->title = "...";
                    t->auther = "...";
                    t->time = 0;
                }
            }

            play_list.push_back(t);
        }
    }

    closedir(dp);
}

static void get_music_lyric(std::string &comment)
{
    try
    {
        if (comment.find("163 key(Don't modify):") == 0)
        {
            LyricParser *data, *tr_data;
            if (music_lyric_163(comment, &data, &tr_data))
            {
                view_music_set_lyric(data, tr_data);
            }
            else
            {
                view_music_set_lyric_state(LYRIC_NONE);
            }
        }
    }
    catch (const std::exception &e)
    {
        LV_LOG_ERROR("%s", e.what());
        view_music_set_lyric_state(LYRIC_FAIL);
    }
}

static void sort_by_pinyin()
{
    boost::locale::generator gen;
    std::locale loc = gen.generate("zh_CN.UTF-8");
    std::collate<wchar_t> const &coll = std::use_facet<std::collate<wchar_t>>(loc);

    auto compare = [&](play_item *a,
                       play_item *b)
    {
        static thread_local std::map<std::string, std::wstring> cache;

        auto get_wstring = [](const std::string &str) -> std::wstring
        {
            auto it = cache.find(str);
            if (it != cache.end())
                return it->second;

            std::wstring result = boost::locale::conv::to_utf<wchar_t>(str, "UTF-8");
            cache[str] = result;
            return result;
        };

        std::wstring wa = get_wstring(a->auther);
        std::wstring wb = get_wstring(b->auther);

        return coll.compare(wa.data(), wa.data() + wa.size(),
                            wb.data(), wb.data() + wb.size()) < 0;
    };

    std::sort(std::execution::par, play_list.begin(), play_list.end(), compare);
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

    if (play_list.empty())
    {
        local_music_scan_now = false;
        return NULL;
    }

    sort_by_pinyin();

    time1 = clock_ms();
    LV_LOG_USER("sort time: %d", time1 - time);
    time = clock_ms();

    for (const auto &item : play_list)
    {
        item->index = play_list_count++;
    }

    std::string name = config::get_config_music_name();
    uint32_t index = config::get_config_music_index();

    play_now_index = 0;

    if (play_list.size() > index)
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
                if (endsWith(item->path, name))
                {
                    play_now_index = item->index;
                    break;
                }
            }
        }
    }

    view_music_init_list();
    local_music_scan_now = false;
    return NULL;
}

void local_music_init()
{
    int res = pthread_create(&rtid, NULL, play_scan_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
    pthread_setname_np(rtid, "play list scan"); 
}

void local_music_run()
{
    pthread_mutex_lock(&play_mutex);
    play_item *item = play_list[play_now_index];

    StreamFile st = StreamFile(item->path);
    music_type type = play_test_music_type(&st);
    if (type == MUSIC_TYPE_UNKNOW)
    {
        LV_LOG_ERROR("Unkown music file type");
        return;
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
        play_st = new StreamNcm(st.copy(), cry);

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
                play_update_image(new data_item(cry->mImageData, cry->imageSize), MUSIC_INFO_IMAGE);
            }

            time_all = cry->mMetaData->duration() / 1000;

            view_music_update_info();
            view_music_update_img();
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