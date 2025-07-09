#ifndef _MUSIC_API_H_
#define _MUSIC_API_H_

#include "http_connect.h"

#include <json/json.hpp>

#include <stdint.h>
#include <vector>

using namespace nlohmann;

typedef enum
{
    NET_MUSIC_NETEASE = 0,
    NET_MUSIC_UNKNOW = -1
} net_music_source;

typedef struct
{
    std::string name;
    std::string artist;
    std::string album;
    std::string image;
    uint64_t id;
    net_music_source source;
} net_music_search_item_t;

typedef struct
{
    std::vector<net_music_search_item_t*> list;
    uint32_t size;
} net_music_search_t;

json api_dynamic_cover(uint64_t lyric_id);
json api_lyric_music(uint64_t lyric_id);
json api_lyric_music_new(uint64_t lyric_id);
json api_url_music(uint64_t url_id);
json api_search_music(uint32_t size, uint32_t page, const char *name);
// json api_image_music(const char *pic_id);

bool api_music_get_search(json &j, net_music_search_t **search);
bool api_music_get_url(json &j, std::string &url, uint32_t *time);
// bool api_music_get_image(json &j, std::string &url);
bool api_music_get_lyric(json &j, std::string &lyric, std::string &tlyric);
bool api_music_get_lyric_new(json &j, std::string &lyric, std::string &tlyric);
bool api_music_get_dynamic_cover(json &j, std::string &url);

void api_music_search_close(net_music_search_t *list);

#endif