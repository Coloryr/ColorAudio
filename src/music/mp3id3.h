#ifndef _MP3ID3_H_
#define _MP3ID3_H_

#include "stream.h"

#include "data_item.h"

#define TITLE_TAG "TIT2"
#define AUTHER_TAG "TPE1"
#define ALBUM_TAG "TALB"
#define TRACK_TAG "TRCK"
#define TIME_TAG "TYER"
#define TCON_TAG "TCON"
#define COMMENT_TAG "COMM"
#define PICTURE_TAG "APIC"

typedef enum {
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

typedef struct
{
    uint8_t version;
    uint8_t r_version;
    uint8_t flag;
    uint32_t length;
    data_item_t title;
    data_item_t album;
    data_item_t auther;
    data_item_t image;
} mp3id3;

bool mp3id3_have(stream_t *st);

mp3id3* mp3id3_read(stream_t *st);

void mp3id3_close(mp3id3* id3);
void mp3id3_skip(stream_t *st);

#endif