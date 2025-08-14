#ifndef _FLAC_METADATA_H_
#define _FLAC_METADATA_H_

#include <stdbool.h>
#include <string>

#include "FLAC++/decoder.h"

#include "stream/stream.h"
#include "common/data_item.h"

#define FLAC_TAG_TITLE "title"
#define FLAC_TAG_ARTLIST "artist"
#define FLAC_TAG_ALBUM "album"
#define FLAC_TAG_DESCRIPTION "DESCRIPTION"

typedef struct
{
    float time;
    std::string title;
    std::string album;
    std::string auther;
    std::string comment;
    coloraudio::common::DataItem* image;
} music_info_t;

namespace coloraudio::flac
{
    class FlacMetadata : FLAC::Decoder::Stream
    {
    public:
        coloraudio::stream::BaseStream *st;
        music_info_t info;

        FlacMetadata(coloraudio::stream::BaseStream *st);
        ~FlacMetadata();

        bool decode_get_info();

    private:
        ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
        ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
        ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
        ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
        bool eof_callback();
        ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[]);
        void metadata_callback(const ::FLAC__StreamMetadata *metadata);
        void error_callback(::FLAC__StreamDecoderErrorStatus status);
    };

}
#endif