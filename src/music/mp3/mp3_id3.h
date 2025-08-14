#ifndef _MP3ID3_H_
#define _MP3ID3_H_

#include <string>

#include "stream/stream.h"
#include "common/data_item.h"

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

namespace coloraudio::mp3
{
    class Mp3Id3
    {
    private:
        coloraudio::stream::BaseStream *st;

    public:
        uint8_t version;
        uint8_t r_version;
        uint8_t flag;
        uint32_t length;
        std::string title;
        std::string album;
        std::string auther;
        std::string comment;
        coloraudio::common::DataItem *image;

        Mp3Id3(coloraudio::stream::BaseStream *st);
        ~Mp3Id3();

        bool get_info();
    };
}

bool mp3id3_have(coloraudio::stream::BaseStream *st);

void mp3id3_skip(coloraudio::stream::BaseStream *st);

#endif