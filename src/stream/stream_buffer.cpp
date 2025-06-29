#include "stream_buffer.h"

#include <malloc.h>

StreamBuffer::StreamBuffer(Stream* st) : stream(st)
{
    buffer = static_cast<uint8_t*>(malloc(STREAM_BUFFER_SIZE));
    buffer_pos = 0;
    buffer_size = STREAM_BUFFER_SIZE;
}

StreamBuffer::~StreamBuffer()
{
    if (buffer)
    {
        free(buffer);
    }
}