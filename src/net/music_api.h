#ifndef _MUSIC_API_H_
#define _MUSIC_API_H_

#include "http_connect.h"

#include <stdint.h>
#include <json-c/json.h>

typedef enum
{
    NET_MUSIC_NETEASE = 0,
    NET_MUSIC_UNKNOW = -1
} net_music_source;

typedef struct
{
    uint8_t *name;
    uint32_t name_size;
    uint8_t *artist;
    uint32_t artist_size;
    uint8_t *album;
    uint32_t album_size;
    uint8_t *pic_id;
    uint64_t id;
    uint64_t url_id;
    uint64_t lyric_id;
    net_music_source source;
} net_music_search_item_t;

typedef struct
{
    net_music_search_item_t* list;
    uint32_t size;
} net_music_search_t;

json_object *api_search_music(http_connect_item_t *connect, uint32_t size, uint32_t page, uint8_t *name);
json_object *api_url_music(http_connect_item_t *connect, uint64_t url_id);
json_object *api_image_music(http_connect_item_t *connect, uint8_t *pic_id);
json_object *api_lyric_music(http_connect_item_t *connect, uint64_t lyric_id);

bool api_music_get_search(json_object *json, net_music_search_t **search);
bool api_music_get_url(json_object *json, uint8_t **url, uint32_t *size, float *br);
bool api_music_get_image(json_object *json, uint8_t **url);
bool api_music_get_lyric(json_object *json, uint8_t **lyric, uint8_t **tlyric);

void api_music_search_close(net_music_search_t *list);

#endif