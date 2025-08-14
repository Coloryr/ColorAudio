#include "stream.h"

using namespace coloraudio::stream;

BaseStream::BaseStream(StreamType type) : stream_type(type)
{
}

BaseStream::~BaseStream()
{
}

uint8_t BaseStream::read_byte()
{
    uint8_t byte;
    if (read(&byte, 1) == 1)
    {
        return byte;
    }
    else
    {
        throw "stream read error";
    }

    return 0;
}
