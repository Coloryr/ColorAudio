#ifndef _PLAY_FLAC_H_
#define _PLAY_FLAC_H_

#include "stream.h"

#include <stdbool.h>

typedef struct
{
    uint8_t *data;
    uint32_t size;
} flac_tag;

typedef struct
{
    float time;
    flac_tag title;
    flac_tag album;
    flac_tag auther;
    flac_tag image;
} flac_metadata;

typedef struct
{
    stream *st;
    flac_metadata metadata;
    bool only_read;
} flac_data;

void flac_close_data(flac_data *flac);
flac_data *flac_read_metadata(stream *st);

bool flac_decode_init(stream *st);
bool flac_decode_start(stream *st);
bool flac_decode_close();

#endif