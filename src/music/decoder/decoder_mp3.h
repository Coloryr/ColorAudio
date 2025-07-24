#ifndef _PLAYER_MP3_H_
#define _PLAYER_MP3_H_

#include "decoder.h"
#include "../stream/stream.h"

#include <stdbool.h>
#include <mad.h>

namespace ColorAudio
{
    class DecoderMp3 : public Decoder
    {
    private:
        struct mad_decoder *decoder;
        uint8_t *buffer;
        uint32_t pos;

    public:
        DecoderMp3(ColorAudio::Stream *st);
        ~DecoderMp3();

        void copy(uint32_t data_size);

        bool decode_start();

        bool can_read() const
        {
            return st->can_read();
        }

        uint8_t* get_buffer() const
        {
            return buffer;
        }

        uint32_t get_pos() const
        {
            return pos;
        }
    };
}

#endif