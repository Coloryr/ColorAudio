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
    public:
        
        struct mad_decoder *decoder;
        uint8_t *buffer;
        uint32_t pos;

        DecoderMp3(ColorAudio::Stream *st);
        ~DecoderMp3();

        bool decode_start();
    };
}

#endif