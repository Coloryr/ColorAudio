#include "net_music.h"

#include "lyric.h"
#include "../net/music_api.h"
#include "../music/mp3_id3.h"
#include "../player/player.h"
#include "../player/player_info.h"
#include "../ui/view.h"
#include "../ui/ui.h"

#include "../lvgl/src/misc/lv_log.h"

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <json/json.hpp>

using namespace nlohmann;
using namespace ColorAudio;

static void *net_pic_run(void *arg)
{
    std::string *pic_url = static_cast<std::string *>(arg);

    // json j = api_image_music(pic_id->c_str());
    // if (j == NULL)
    // {
    //     LV_LOG_ERROR("JSON parse error");
    //     return NULL;
    // }

    // std::string pic_url;
    // if (!api_music_get_image(j, pic_url))
    // {
    //     LV_LOG_ERROR("image url get error");
    //     return NULL;
    // }

    data_item *item = http_get_data(*pic_url);

    if (item != NULL)
    {
        play_update_image(item, MUSIC_INFO_IMAGE);
        view_update_img();
    }

    return NULL;
}

static void *net_lyric_run(void *arg)
{
    uint64_t *lyric_id = (uint64_t *)arg;

    json j = api_lyric_music(*lyric_id);

    if (j == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        return NULL;
    }

    std::string lyric, tlyric;
    if (api_music_get_lyric(j, lyric, tlyric))
    {
        lyric_node_t *data = parse_memory_lrc(const_cast<char *>(lyric.c_str()));
        lyric_node_t *tr_data = parse_memory_lrc(const_cast<char *>(tlyric.c_str()));

        view_set_lyric(data, tr_data);
    }

    return NULL;
}

static net_music_search_t *get_search_list(uint32_t size, uint32_t page, const char *name)
{
    json j = api_search_music(20, 1, "Daisy Crown (English Ver.)");
    if (j == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        return NULL;
    }

    net_music_search_t *list = NULL;
    if (!api_music_get_search(j, &list))
    {
        LV_LOG_ERROR("JSON parse error");
        return NULL;
    }

    return list;
}

static bool get_play_url(uint64_t id, std::string &url, uint32_t *time)
{
    json j1 = api_url_music(id);
    if (j1 == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        return false;
    }
    if (!api_music_get_url(j1, url, time))
    {
        LV_LOG_ERROR("JSON parse error");
        return false;
    }

    return true;
}

static void *play_run(void *arg)
{
    net_music_search_t *list = get_search_list(20, 1, "Daisy Crown (English Ver.)");

    while (list == NULL || list->list.size() == 0)
    {
        list = get_search_list(20, 1, "Daisy Crown (English Ver.)");
        usleep(5000000);
    }

    net_music_search_item_t *item = list->list[0];

    play_update_text(item->name, MUSIC_INFO_TITLE);
    play_update_text(item->artist, MUSIC_INFO_AUTHER);
    play_update_text(item->album, MUSIC_INFO_ALBUM);

    view_update_info();

    pthread_t pid1, pid2;
    pthread_create(&pid1, NULL, net_pic_run, &item->image);
    pthread_create(&pid2, NULL, net_lyric_run, &item->id);

    std::string url;
    uint32_t time;
    for (uint8_t i = 0; i < 10; i++)
    {
        if (get_play_url(item->id, url, &time))
        {
            break;
        }
    }

    HttpStream http = HttpStream(url);

    if (http.connect())
    {
        Stream *st = new StreamHttp(&http);

        music_type type = play_test_music_type(st);
        if (type == MUSIC_TYPE_UNKNOW)
        {
            LV_LOG_ERROR("Unkown music file type");
            delete st;
            return NULL;
        }

        if (type == MUSIC_TYPE_MP3)
        {
            mp3_id3 id3 = mp3_id3(st);
            id3.get_info();

            time_all = (float)time / 1000;
        }

        view_update_info();

        play_st = st;
        pthread_cond_signal(&play_start);
        pthread_mutex_unlock(&play_mutex);

        usleep(1000);

        // 等待播放结束
        pthread_mutex_lock(&play_mutex);
    }

    if (list)
    {
        delete list;
    }

    return NULL;
}

void net_music_init()
{
    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
}