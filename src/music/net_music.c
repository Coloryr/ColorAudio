#include "net_music.h"

#include "http_connect.h"
#include "music_api.h"
#include "data_item.h"
#include "player.h"
#include "view.h"
#include "lyric.h"

#include "lvgl/src/misc/lv_log.h"

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <semaphore.h>

static http_connect_item_t *http_connect;

static void *net_pic_run(void *arg)
{
    uint8_t *pic_id = (uint8_t *)arg;
    http_connect_item_t *connect = http_create_connect();

    json_object *json = api_image_music(connect, pic_id);
    if (json == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        goto connect_exit;
    }

    uint8_t *pic_url = NULL;
    if (!api_music_get_image(json, &pic_url))
    {
        LV_LOG_ERROR("image url get error");
        goto json_exit;
    }

    http_url(connect, pic_url);
    http_get(connect);

    data_item_t *item;
    http_start_data(connect, &item);

    if (http_run(connect))
    {
        play_info_update_raw(item->data, item->size, MUSIC_INFO_IMAGE);
        view_update_img();
    }

    data_item_close(item);
    free(pic_url);
json_exit:
    json_object_put(json);
connect_exit:
    http_free(connect);
}

static void *net_lyric_run(void *arg)
{
    uint64_t *lyric_id = (uint64_t *)arg;
    http_connect_item_t *connect = http_create_connect();

    json_object *json = api_lyric_music(connect, *lyric_id);

    if (json == NULL)
    {
        goto http_exit;
    }

    data_item_t *item;
    http_start_data(connect, &item);

    uint8_t *lyric, *tlyric;
    if (api_music_get_lyric(json, &lyric, &tlyric))
    {
        lyric_node_t *data = parse_memory_lrc(lyric);
        lyric_node_t *tr_data = parse_memory_lrc(tlyric);

        view_set_lyric(data, tr_data);
    }
    data_item_close(item);
json_exit:
    json_object_put(json);
http_exit:
    http_free(connect);
}

static void *play_run(void *arg)
{
    json_object *json = api_search_music(http_connect, 20, 1, "Daisy Crown (English Ver.)");
    if (json == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        return NULL;
    }

    net_music_search_t *list = NULL;
    if (!api_music_get_search(json, &list))
    {
        LV_LOG_ERROR("JSON parse error");
        json_object_put(json);
        return NULL;
    }

    json_object_put(json);

    if (list == NULL)
    {
        return NULL;
    }

    net_music_search_item_t *item = &list->list[0];

    if (item == NULL)
    {
        return NULL;
    }

    play_info_update_raw(item->name, item->name_size, MUSIC_INFO_TITLE);
    play_info_update_raw(item->artist, item->artist_size, MUSIC_INFO_AUTHER);
    play_info_update_raw(item->album, item->album_size, MUSIC_INFO_ALBUM);

    view_update_info();

    pthread_t pid1, pid2;
    pthread_create(&pid1, NULL, net_pic_run, item->pic_id);
    pthread_create(&pid2, NULL, net_lyric_run, &item->lyric_id);

    json = api_url_music(http_connect, item->url_id);
    if (json == NULL)
    {
        LV_LOG_ERROR("JSON parse error");
        return NULL;
    }

    uint8_t *url = NULL;
    uint32_t size;
    float br;
    if (!api_music_get_url(json, &url, &size, &br))
    {
        LV_LOG_ERROR("JSON parse error");
        api_music_search_close(list);
        json_object_put(json);
        return NULL;
    }

    json_object_put(json);

    http_connect_item_t *connect = http_create_connect();

    stream_t *st;
    http_start_stream(connect, url, &st);

    music_type type = play_test_music_type(st);
    if (type == MUSIC_TYPE_UNKNOW)
    {
        LV_LOG_ERROR("Unkown music file type");
        stream_close(st);
        return NULL;
    }

    if (type == MUSIC_TYPE_MP3)
    {
        mp3id3 *id3 = mp3id3_read(st);
        if (id3)
        {
            mp3id3_close(id3);
        }

        time_all = (float)size * 8 / br / 1000;
    }

    play_st = st;
    pthread_cond_signal(&play_start);
    pthread_mutex_unlock(&play_mutex);

    usleep(1000);

    // 等待播放结束
    pthread_mutex_lock(&play_mutex);

    // 关闭
    stream_close(st);

    free(url);
}

void net_music_init()
{
    http_connect = http_create_connect();

    pthread_t rtid;
    int res = pthread_create(&rtid, NULL, play_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play list read thread run fail: %d", res);
    }
}