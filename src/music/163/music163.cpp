#include <string>

#include <json/json.hpp>

#include "music/lyric.h"
#include "net/music_api.h"
#include "ui/music_view.h"

#include "cryp163.h"
#include "music163.h"

using namespace coloraudio::lyric;

bool music_lyric_163(uint64_t id, LyricParser **ldata, LyricParser **trdata)
{
    json j = api_lyric_music_new(id);

    std::string lyric, tlyric;
    if (api_music_get_lyric_new(j, lyric, tlyric))
    {
        *ldata = new LyricParser(lyric);
        *trdata = new LyricParser(tlyric);

        return true;
    }
    else
    {
        j = api_lyric_music(id);

        std::string lyric, tlyric;
        if (api_music_get_lyric(j, lyric, tlyric))
        {
            *ldata = new LyricParser(lyric);
            *trdata = new LyricParser(tlyric);

            return true;
        }
        else
        {
            return false;
        }
    }
}

bool music_lyric_163(std::string &comment, LyricParser **ldata, LyricParser **trdata)
{
    std::string key = comment.substr(22);
    std::string temp = dep(key);
    if (temp.empty())
    {
        return false;
    }
    temp = temp.substr(6);
    json j1 = json::parse(temp);
    json id = j1["musicId"];
    if (id.is_string())
    {
        uint64_t id1 = std::stoul(id.get<std::string>());

        return music_lyric_163(id1, ldata, trdata);
    }

    return false;
}
