#ifndef _MP3ID3_H_
#define _MP3ID3_H_

#include "../stream/stream.h"

#include "../common/data_item.h"

#include <string>

#define TITLE_TAG "TIT2"
#define AUTHER_TAG "TPE1"
#define ALBUM_TAG "TALB"
#define TRACK_TAG "TRCK"
#define TIME_TAG "TYER"
#define TCON_TAG "TCON"
#define COMMENT_TAG "COMM"
#define PICTURE_TAG "APIC"

typedef enum
{
    ID3_TITLE_TAG = 0,
    ID3_AUTHER_TAG,
    ID3_ALBUM_TAG,
    ID3_TRACK_TAG,
    ID3_TIME_TAG,
    ID3_TCON_TAG,
    ID3_COMMENT_TAG,
    ID3_PICTURE_TAG,
    ID3_UNKNOW_TAG = -1
} id3_type;

namespace ColorAudio
{
    class mp3_id3
    {
    private:
        ColorAudio::Stream *st;

    public:
        uint8_t version;
        uint8_t r_version;
        uint8_t flag;
        uint32_t length;
        std::string title;
        std::string album;
        std::string auther;
        std::string comment;
        data_item *image;

        mp3_id3(ColorAudio::Stream *st);
        ~mp3_id3();

        bool get_info();
    };
}

bool mp3id3_have(ColorAudio::Stream *st);

void mp3id3_skip(ColorAudio::Stream *st);

#endif