#ifndef _STREAM_BUFFER_H_
#define _STREAM_BUFFER_H_

#include "stream.h"

#include <stdbool.h>

namespace ColorAudio
{
    class StreamBuffer
    {
    private:
        Stream *stream;
        uint8_t *buffer;
        uint32_t buffer_pos;
        uint32_t buffer_size;

    public:
        StreamBuffer(Stream *st);
        ~StreamBuffer();
    };
}

#endif