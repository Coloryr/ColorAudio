#include "stream_mem.h"

#include "../lvgl/src/misc/lv_log.h"

#include <string.h>
#include <stdio.h>

using namespace ColorAudio;

StreamMemory::StreamMemory(uint8_t *buffer, uint32_t size) : Stream(STREAM_TYPE_MEM),
   buffer_mem(buffer),
   buffer_pos(0),
   buffer_size(size)
{
    if (buffer == nullptr || size == 0)
    {
        LV_LOG_ERROR("[stream] Can't use NULL memory or size is 0");
    }
}

StreamMemory::StreamMemory(data_item *item) : Stream(STREAM_TYPE_MEM),
    buffer_mem(item->data),
    buffer_pos(0),
    buffer_size(item->size)
{

}

StreamMemory::~StreamMemory()
{

}

uint32_t StreamMemory::read(uint8_t* buffer, uint32_t len)
{
    if (buffer_pos + len > buffer_size)
    {
        len = buffer_size - buffer_pos;
    }
    memcpy(buffer, buffer_mem + buffer_pos, len);
    buffer_pos += len;
    return len;
}

uint32_t StreamMemory::write(uint8_t* buffer, uint32_t len)
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

uint32_t StreamMemory::peek(uint8_t* buffer, uint32_t len)
{
    if (buffer_pos + len > buffer_size)
    {
        len = buffer_size - buffer_pos;
    }
    memcpy(buffer, buffer_mem + buffer_pos, len);
    return len;
}

uint32_t StreamMemory::get_pos()
{
    return buffer_pos;
}

uint32_t StreamMemory::get_all_size()
{
    return buffer_size;
}

uint32_t StreamMemory::get_less_read()
{
    return buffer_size - buffer_pos;
}

void StreamMemory::seek(int32_t pos, uint8_t where)
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

bool StreamMemory::test_read_size(uint32_t size)
{
    return buffer_pos + size <= buffer_size;
}

bool StreamMemory::can_read()
{
    return buffer_pos < buffer_size;
}