#ifndef _PLAY_FLAC_H_
#define _PLAY_FLAC_H_

#include "stream.h"

#include "data_item.h"

#include <stdbool.h>
#include <string>

typedef struct
{
    float time;
    std::string title;
    std::string album;
    std::string auther;
    DataItem image;
} flac_metadata_t;

typedef struct
{
    Stream *st;
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    flac_metadata_t metadata;
    bool metadata_only;
} flac_data_t;

class DecoderFlac
{
public:
	DecoderFlac();
	~DecoderFlac();

private:

};

DecoderFlac::DecoderFlac()
{
}

DecoderFlac::~DecoderFlac()
{
}

void flac_data_close(flac_data_t *flac);
flac_data_t *flac_read_metadata(stream_t *st);

bool flac_decode_init(stream_t *st);
bool flac_decode_start(stream_t *st);
bool flac_decode_close();

#endif