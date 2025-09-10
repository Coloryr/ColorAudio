#include <string.h>
#include <stdio.h>

#include "lvgl/src/misc/lv_log.h"

#include "stream_mem.h"

using namespace coloraudio::stream;
using namespace coloraudio::common;

MemoryStream::MemoryStream(uint8_t *buffer, uint32_t size)
    : BaseStream(STREAM_TYPE_MEM),
      buffer_mem(buffer),
      buffer_size(size)
{
    if (buffer == nullptr || size == 0)
    {
        throw "[stream] NULL memory or size is zero";
    }
}

MemoryStream::MemoryStream(DataItem *item) : BaseStream(STREAM_TYPE_MEM)
{
    if (item == nullptr)
    {
        throw "[stream] item is null";
    }

    buffer_mem = item->get_data();
    buffer_size = item->get_size();
}

MemoryStream::~MemoryStream()
{
}

uint32_t MemoryStream::read(uint8_t *buffer, uint32_t len)
{
    if (buffer_pos + len > buffer_size)
    {
        len = buffer_size - buffer_pos;
    }
    memcpy(buffer, buffer_mem + buffer_pos, len);
    buffer_pos += len;
    return len;
}

uint32_t MemoryStream::write(uint8_t *buffer, uint32_t len)
{
    uint32_t remain = buffer_size - buffer_pos;
    if (len > remain)
    {
        len = remain;
    }
    memcpy(buffer_mem + buffer_pos, buffer, len);
    buffer_pos += len;
    return len;
}

uint32_t MemoryStream::peek(uint8_t *buffer, uint32_t len)
{
    if (buffer_pos + len > buffer_size)
    {
        len = buffer_size - buffer_pos;
    }
    memcpy(buffer, buffer_mem + buffer_pos, len);
    return len;
}

uint32_t MemoryStream::get_pos()
{
    return buffer_pos;
}

uint32_t MemoryStream::get_all_size()
{
    return buffer_size;
}

uint32_t MemoryStream::get_less_read()
{
    return buffer_size - buffer_pos;
}

void MemoryStream::seek(int32_t pos, uint8_t where)
{
    switch (where)
    {
    case SEEK_CUR:
        if (buffer_pos + pos > buffer_size)
        {
            buffer_pos = buffer_size;
        }
        else
        {
            buffer_pos += pos;
        }
        break;
    case SEEK_SET:
        if (pos > buffer_size)
        {
            buffer_pos = buffer_size;
        }
        else
        {
            buffer_pos = pos;
        }
        break;
    case SEEK_END:
        int32_t new_pos = buffer_size + pos;
        if (new_pos < 0)
            new_pos = 0;
        if (new_pos > buffer_size)
            new_pos = buffer_size;
        buffer_pos = new_pos;
        break;
    }
}

bool MemoryStream::test_read_size(uint32_t size)
{
    return buffer_pos + size <= buffer_size;
}

bool MemoryStream::can_read()
{
    return buffer_pos < buffer_size;
}