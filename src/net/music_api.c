#include "music_api.h"

#include "http_connect.h"

#include <json-c/json.h>
#include <string.h>

#define MUSIC_API "https://music.mcya.cn/api.php"
#define MUSIC_API_SEARCH_ARG "types=search&count=%d&source=netease&pages=%d&name=%s"
#define MUSIC_API_URL_ARG "types=url&source=netease&id=%ld"
#define MUSIC_API_PIC_ARG "types=pic&source=netease&id=%s"
#define MUSIC_API_LYRIC_ARG "types=lyric&source=netease&id=%ld"

static http_connect_item_t *http_connect;

json_object *api_lyric_music(http_connect_item_t *connect, uint64_t lyric_id)
{
    uint8_t data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_LYRIC_ARG, lyric_id);

    http_url(connect, data);
    http_get(connect);
    data_item_t *item = NULL;
    json_object *json = NULL;
    http_start_data(connect, &item);
    if (http_run(connect))
    {
        json = json_tokener_parse(item->data);
    }

    data_item_close(item);

    return json;
}

json_object *api_url_music(http_connect_item_t *connect, uint64_t url_id)
{
    uint8_t data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_URL_ARG, url_id);

    http_url(connect, data);
    http_get(connect);
    data_item_t *item = NULL;
    json_object *json = NULL;
    http_start_data(connect, &item);
    if (http_run(connect))
    {
        json = json_tokener_parse(item->data);
    }

    data_item_close(item);

    return json;
}

json_object *api_search_music(http_connect_item_t *connect, uint32_t size, uint32_t page, uint8_t *name)
{
    char *encoded = http_encode_param(connect, name);
    uint8_t data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_SEARCH_ARG, size, page, encoded);

    http_url(connect, data);
    http_get(connect);
    data_item_t *item = NULL;
    json_object *json = NULL;
    http_start_data(connect, &item);
    if (http_run(connect))
    {
        json = json_tokener_parse(item->data);
    }

    http_free_data(encoded);
    data_item_close(item);

    return json;
}

void api_music_search_close(net_music_search_t *list)
{
    for (size_t i = 0; i < list->size; i++)
    {
        free(list->list[i].name);
        free(list->list[i].artist);
        free(list->list[i].album);
        free(list->list[i].pic_id);
    }

    free(list->list);
    free(list);
}

bool api_music_get_url(json_object *json, uint8_t **url, uint32_t *size, float *br)
{
    if (json == NULL)
    {
        return false;
    }

    if (!json_object_is_type(json, json_type_object))
    {
        return false;
    }

    json_object *url_obj, *size_obj, *br_obj;
    json_object_object_get_ex(json, "url", &url_obj);
    json_object_object_get_ex(json, "size", &size_obj);
    json_object_object_get_ex(json, "br", &br_obj);

    if (url_obj == NULL)
    {
        return false;
    }

    const char *item = url_obj ? json_object_get_string(url_obj) : "";
    uint32_t size1 = strlen(item) + 1;
    *url = malloc(size1);
    memcpy(*url, (uint8_t *)item, size1);

    if (size_obj != NULL)
    {
        *size = json_object_get_int(size_obj);
    }

    if (br_obj != NULL)
    {
        *br = (float)json_object_get_double(br_obj);
    }

    return true;
}

bool api_music_get_image(json_object *json, uint8_t **url)
{
    if (json == NULL)
    {
        return false;
    }

    if (!json_object_is_type(json, json_type_object))
    {
        return false;
    }

    json_object *url_obj;
    json_object_object_get_ex(json, "url", &url_obj);

    if (url_obj == NULL)
    {
        return false;
    }

    const char *item = url_obj ? json_object_get_string(url_obj) : "";
    uint32_t size1 = strlen(item) + 1;
    *url = malloc(size1);
    memcpy(*url, (uint8_t *)item, size1);

    return true;
}

bool api_music_get_search(json_object *json, net_music_search_t **search)
{
    if (json == NULL)
    {
        return NULL;
    }

    if (!json_object_is_type(json, json_type_array))
    {
        return NULL;
    }

    net_music_search_t *res = malloc(sizeof(net_music_search_t));
    *search = res;
    uint8_t data[512];
    uint32_t item_size = 0;
    const char *item;
    uint64_t item1;

    int size = json_object_array_length(json); // 获取数组长度

    res->size = size;
    res->list = calloc(size, sizeof(net_music_search_item_t));

    for (int i = 0; i < size; i++)
    {
        // 获取数组元素
        json_object *item_obj = json_object_array_get_idx(json, i);

        // 提取对象字段
        json_object *name_obj, *id_obj, *ats_obj, *album_obj, *pic_id_obj, *url_id_obj, *lyric_id_obj;
        json_object_object_get_ex(item_obj, "name", &name_obj);
        json_object_object_get_ex(item_obj, "id", &id_obj);
        json_object_object_get_ex(item_obj, "artist", &ats_obj);
        json_object_object_get_ex(item_obj, "album", &album_obj);
        json_object_object_get_ex(item_obj, "pic_id", &pic_id_obj);
        json_object_object_get_ex(item_obj, "url_id", &url_id_obj);
        json_object_object_get_ex(item_obj, "lyric_id", &lyric_id_obj);

        data[0] = 0;

        if (ats_obj && json_object_is_type(ats_obj, json_type_array))
        {
            int artist_count = json_object_array_length(ats_obj);

            for (int j = 0; j < artist_count; j++)
            {
                json_object *artist_item = json_object_array_get_idx(ats_obj, j);
                const char *artist_str = json_object_get_string(artist_item);

                if (j > 0)
                    strcat(data, "/");
                strcat(data, artist_str);
            }

            item_size = strlen(data) + 1;
            res->list[i].artist_size = item_size;
            res->list[i].artist = malloc(item_size);
            memcpy(res->list[i].artist, data, item_size);
        }

        item = name_obj ? json_object_get_string(name_obj) : "N/A";
        item_size = strlen(item) + 1;
        res->list[i].name_size = item_size;
        res->list[i].name = malloc(item_size);
        memcpy(res->list[i].name, (uint8_t *)item, item_size);

        item = album_obj ? json_object_get_string(album_obj) : "N/A";
        item_size = strlen(item) + 1;
        res->list[i].album_size = item_size;
        res->list[i].album = malloc(item_size);
        memcpy(res->list[i].album, (uint8_t *)item, item_size);

        item = pic_id_obj ? json_object_get_string(pic_id_obj) : "0";
        item_size = strlen(item) + 1;
        res->list[i].pic_id = malloc(item_size);
        memcpy(res->list[i].pic_id, (uint8_t *)item, item_size);

        res->list[i].id = id_obj ? json_object_get_int64(id_obj) : 0;
        res->list[i].url_id = url_id_obj ? json_object_get_int64(url_id_obj) : 0;
        res->list[i].lyric_id = lyric_id_obj ? json_object_get_int64(lyric_id_obj) : 0;

        res->list[i].source = NET_MUSIC_NETEASE;
    }

    return true;
}

json_object *api_image_music(http_connect_item_t *connect, uint8_t *pic_id)
{
    uint8_t data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_PIC_ARG, pic_id);

    http_url(connect, data);
    http_get(connect);
    data_item_t *item = NULL;
    json_object *json = NULL;
    http_start_data(connect, &item);
    if (http_run(connect))
    {
        json = json_tokener_parse(item->data);
    }

    data_item_close(item);

    return json;
}

bool api_music_get_lyric(json_object *json, uint8_t **lyric, uint8_t **tlyric)
{
    if (json == NULL)
    {
        return false;
    }

    if (!json_object_is_type(json, json_type_object))
    {
        return false;
    }

    json_object *lyric_obj, *tlyric_obj;
    json_object_object_get_ex(json, "lyric", &lyric_obj);
    json_object_object_get_ex(json, "tlyric", &tlyric_obj);

    const char *item = lyric_obj ? json_object_get_string(lyric_obj) : "";
    uint32_t size1 = strlen(item) + 1;
    *lyric = malloc(size1);
    memcpy(*lyric, (uint8_t *)item, size1);

    item = tlyric_obj ? json_object_get_string(tlyric_obj) : "";
    size1 = strlen(item) + 1;
    *tlyric = malloc(size1);
    memcpy(*tlyric, (uint8_t *)item, size1);

    return true;
}