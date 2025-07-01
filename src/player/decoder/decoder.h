#ifndef _DECODER_H_
#define _DECODER_H_

#include "../stream/stream.h"
#include "../common/data_item.h"

#include <stdbool.h>
#include <string>

namespace ColorAudio
{
    class Decoder
    {
    public:
         ColorAudio::Stream *st;

        Decoder(ColorAudio::Stream *st);
        ~Decoder();

        virtual bool decode_start() = 0;
    };
}

#endif