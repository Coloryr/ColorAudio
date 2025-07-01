#ifndef _PLAY_FLAC_H_
#define _PLAY_FLAC_H_

#include "decoder.h"
#include "../stream/stream.h"
#include "../common/data_item.h"

#include "FLAC++/decoder.h"

#include <stdbool.h>
#include <string>

namespace ColorAudio
{
    class DecoderFlac : public Decoder, FLAC::Decoder::Stream
    {
    public:
        uint32_t sample_rate;
        uint32_t bits_per_sample;

        DecoderFlac(ColorAudio::Stream *st);
        ~DecoderFlac();

        bool decode_start();

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