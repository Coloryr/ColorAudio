#include "music_api.h"

#include "http_connect.h"

#include <json/json.hpp>
#include <string.h>

#include <string>

#define MUSIC_API "https://music.mcya.cn/api.php"
#define MUSIC_API_SEARCH_ARG "types=search&count=%d&source=netease&pages=%d&name=%s"
#define MUSIC_API_URL_ARG "types=url&source=netease&id=%ld"
#define MUSIC_API_PIC_ARG "types=pic&source=netease&id=%s"
#define MUSIC_API_LYRIC_ARG "types=lyric&source=netease&id=%ld"

using namespace nlohmann;

json api_lyric_music(uint64_t lyric_id)
{
    char data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_LYRIC_ARG, lyric_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_url_music(uint64_t url_id)
{
    char data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_URL_ARG, url_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_search_music(uint32_t size, uint32_t page, const char *name)
{
    char data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_SEARCH_ARG, size, page, name);

    std::string res = http_get_string(data);
    return json::parse(res);
}

json api_image_music(const char *pic_id)
{
    char data[512];
    sprintf(data, MUSIC_API "?" MUSIC_API_PIC_ARG, pic_id);

    std::string res = http_get_string(data);
    return json::parse(res);
}

void api_music_search_close(net_music_search_t *list)
{
    for (int i = 0; i < list->list.size(); ++i)
    {
        delete list->list[i];
    }
    list->list.clear();
    delete list;
}

bool api_music_get_url(json &json, std::string &url, uint32_t *size, float *br)
{
    if (json == NULL)
    {
        return false;
    }

    if (!json.is_object())
    {
        return false;
    }

    auto temp = json["url"];
    if (temp.is_string())
    {
        url = temp.get<std::string>();
    }
    temp = json["size"];
    if (temp.is_string())
    {
        *size = temp.get<uint32_t>();
    }
    temp = json["br"];
    if (temp.is_string())
    {
        *br = temp.get<float>();
    }

    return true;
}

bool api_music_get_image(json &j, std::string &url)
{
    if (j == NULL)
    {
        return false;
    }

    if (!j.is_object())
    {
        return false;
    }

    auto temp = j["url"];
    if (temp.is_string())
    {
        url = temp.get<std::string>();
    }

    return true;
}

bool api_music_get_search(json &j, net_music_search_t **search)
{
    if (j == NULL)
    {
        return false;
    }

    if (!j.is_array())
    {
        return false;
    }

    net_music_search_t *res = new net_music_search_t();

    *search = res;

    res->size = j.size();
    res->list.reserve(res->size);

    uint16_t i = 0;
    for (json::iterator it = j.begin(); it != j.end(); ++it)
    {
        net_music_search_item_t *item = new net_music_search_item_t();

        // 提取对象字段
        json name_obj = (*it)["name"];
        json id_obj = (*it)["id"];
        json ats_obj = (*it)["artist"];
        json album_obj = (*it)["album"];
        json pic_id_obj = (*it)["pic_id"];
        json url_id_obj = (*it)["url_id"];
        json lyric_id_obj = (*it)["lyric_id"];

        if (ats_obj.is_array())
        {
            std::string artist_str;
            uint8_t j = 0;
            for (json::iterator it1 = ats_obj.begin(); it1 != ats_obj.end(); ++it1)
            {
                if (j > 0)
                {
                    artist_str.append("/");
                }
                artist_str.append((*it1).get<std::string>());
                j++;
            }

            item->artist = artist_str;
        }

        item->name = name_obj.is_string() ? name_obj.get<std::string>() : "N/A";
        item->album = album_obj.is_string() ? album_obj.get<std::string>() : "N/A";
        item->pic_id = pic_id_obj.is_string() ? pic_id_obj.get<std::string>() : "0";
        item->id = id_obj.is_number() ? id_obj.get<uint64_t>() : 0;
        item->url_id = url_id_obj.is_number() ? url_id_obj.get<uint64_t>() : 0;
        item->lyric_id = lyric_id_obj.is_number() ? lyric_id_obj.get<uint64_t>() : 0;

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

    json lyric_obj = j["lyric"];
    json tlyric_obj = j["tlyric"];

    lyric = lyric_obj.is_string() ? lyric_obj.get<std::string>() : "";
    tlyric = tlyric_obj.is_string() ? tlyric_obj.get<std::string>() : "";

    return true;
}