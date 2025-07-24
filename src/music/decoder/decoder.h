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

    protected:
        ColorAudio::Stream *st;

    public:
        Decoder(ColorAudio::Stream *st);
        virtual ~Decoder();

        virtual bool decode_start() = 0;
    };
}

#endif