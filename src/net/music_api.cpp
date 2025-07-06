#include "music_api.h"

#include "http_connect.h"

#include <json/json.hpp>
#include <string.h>

#include <string>

#define MUSIC_API "https://zm.armoe.cn"
#define MUSIC_API_SEARCH_ARG "/cloudsearch?limit=%d&offset=%d&keywords=%s"
#if BUILD_ARM
#define MUSIC_API_URL_ARG "/song/url?id=%lld"
#define MUSIC_API_LYRIC_ARG "/lyric?id=%lld"
#define MUSIC_API_DYNAMIC_COVER_ARG "/song/dynamic/cover?id=%lld"
#else
#define MUSIC_API_URL_ARG "/song/url?id=%ld"
#define MUSIC_API_LYRIC_ARG "/lyric?id=%ld"
#define MUSIC_API_DYNAMIC_COVER_ARG "/song/dynamic/cover?id=%ld"
#endif

using namespace nlohmann;

json api_dynamic_cover(uint64_t lyric_id)
{
    char data[512];

    sprintf(data, MUSIC_API MUSIC_API_DYNAMIC_COVER_ARG, lyric_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_lyric_music(uint64_t lyric_id)
{
    char data[512];

    sprintf(data, MUSIC_API MUSIC_API_LYRIC_ARG, lyric_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_url_music(uint64_t url_id)
{
    char data[512];
    sprintf(data, MUSIC_API MUSIC_API_URL_ARG, url_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_search_music(uint32_t size, uint32_t page, const char *name)
{
    char data[512];
    sprintf(data, MUSIC_API MUSIC_API_SEARCH_ARG, size, (page - 1) * size, name);

    std::string res = http_get_string(data);
    return json::parse(res);
}

// json api_image_music(const char *pic_id)
// {
//     char data[512];
//     sprintf(data, MUSIC_API "?" MUSIC_API_PIC_ARG, pic_id);

//     std::string res = http_get_string(data);
//     return json::parse(res);
// }

void api_music_search_close(net_music_search_t *list)
{
    for (int i = 0; i < list->list.size(); ++i)
    {
        delete list->list[i];
    }
    list->list.clear();
    delete list;
}

bool api_music_get_url(json &j, std::string &url, uint32_t *time)
{
    if (j == NULL || !j.is_object())
    {
        return false;
    }

    json j1 = j["data"][0];

    if (!j1.is_object())
    {
        return false;
    }

    auto temp = j1["url"];
    if (temp.is_string())
    {
        url = temp.get<std::string>();
    }
    temp = j1["time"];
    if (temp.is_string())
    {
        *time = temp.get<uint32_t>();
    }

    return true;
}

// bool api_music_get_image(json &j, std::string &url)
// {
//     if (j == NULL)
//     {
//         return false;
//     }

//     if (!j.is_object())
//     {
//         return false;
//     }

//     auto temp = j["data"][0]["url"];
//     if (temp.is_string())
//     {
//         url = temp.get<std::string>();
//     }

//     return true;
// }

bool api_music_get_dynamic_cover(json &j, std::string &url)
{
    if (j == NULL)
    {
        return false;
    }

    if (!j.is_object())
    {
        return false;
    }

    auto temp = j["data"]["videoPlayUrl"];
    if (temp.is_string())
    {
        url = temp.get<std::string>();
        return true;
    }

    return false;
}

bool api_music_get_search(json &j, net_music_search_t **search)
{
    if (j == NULL || !j.is_object())
    {
        return false;
    }

    json j1 = j["result"]["songs"];

    if (!j1.is_array())
    {
        return false;
    }

    net_music_search_t *res = new net_music_search_t();

    *search = res;

    res->size = j1.size();
    res->list.reserve(res->size);

    uint16_t i = 0;
    for (const auto &pair : j1)
    {
        net_music_search_item_t *item = new net_music_search_item_t();

        // 提取对象字段
        json name_obj = pair["name"];
        json id_obj = pair["id"];
        json ats_obj = pair["ar"];
        json album_obj = pair["al"]["name"];
        json img_obj = pair["al"]["picUrl"];

        if (ats_obj.is_array())
        {
            std::string artist_str;
            uint8_t j = 0;
            for (const auto &pair1 : ats_obj)
            {
                if (j > 0)
                {
                    artist_str.append("/");
                }
                artist_str.append(pair1["name"].get<std::string>());
                j++;
            }

            item->artist = artist_str;
        }

        item->name = name_obj.is_string() ? name_obj.get<std::string>() : "";
        item->album = album_obj.is_string() ? album_obj.get<std::string>() : "";
        item->image = img_obj.is_string() ? img_obj.get<std::string>() : "";
        item->id = id_obj.is_number() ? id_obj.get<uint64_t>() : 0;

        item->source = NET_MUSIC_NETEASE;

        i++;

        res->list.push_back(item);
    }

    return true;
}

bool api_music_get_lyric(json &j, std::string &lyric, std::string &tlyric)
{
    if (j == NULL)
    {
        return false;
    }

    if (!j.is_object())
    {
        return false;
    }

    json lyric_obj = j["lrc"]["lyric"];
    json tlyric_obj = j["tlyric"]["lyric"];

    lyric = lyric_obj.is_string() ? lyric_obj.get<std::string>() : "";
    tlyric = tlyric_obj.is_string() ? tlyric_obj.get<std::string>() : "";

    return true;
}