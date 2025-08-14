#ifndef _DECODER_H_
#define _DECODER_H_

#include <stdbool.h>
#include <string>

#include "stream/stream.h"
#include "common/data_item.h"

namespace coloraudio::decoder
{
    class Decoder
    {

    protected:
        coloraudio::stream::BaseStream *st;

    public:
        Decoder(coloraudio::stream::BaseStream *st);
        virtual ~Decoder();

        virtual bool decode_start() = 0;
    };
}

#endif