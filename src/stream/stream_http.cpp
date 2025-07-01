#include "stream_http.h"

#include "../common/utils.h"

#include <malloc.h>
#include <string.h>

using namespace ColorAudio;

StreamHttp::StreamHttp(IStreamHttp *http) : Stream(STREAM_TYPE_HTTP),
                                            http(http)
{
    buffer = static_cast<uint8_t *>(malloc(STREAM_BUFFER_SIZE));
    buffer_size = STREAM_BUFFER_SIZE;
    if (buffer == nullptr)
    {
        return;
    }

    http_size = http->get_size();
    http_pos = 0;
}

StreamHttp::~StreamHttp()
{
    if (buffer)
    {
        free(buffer);
        buffer = nullptr;
    }

    http->close();
}

void StreamHttp::read_block()
{
    memcpy(buffer, buffer + buffer_pos, buffer_write);
    buffer_pos = 0;

    uint32_t size = min(buffer_size - buffer_write, buffer_size);
    uint32_t rsize = http->read(buffer + buffer_write, size);
    http_pos += rsize;
    if (rsize == 0 && http_pos != http_size)
    {
        rsize = http->read(buffer + buffer_pos, size);
        http_pos += rsize;
    }
    if (size != rsize)
    {
        if (http_pos != http_size)
        {
            throw "size error";
        }
        is_eof = true;
    }
    buffer_write += rsize;
}

uint32_t StreamHttp::buffer_read(uint8_t *buf, uint32_t len)
{
    if (buffer_write >= len)
    {
        memcpy(buf, buffer + buffer_pos, len);
        buffer_pos += len;
        buffer_write -= len;
        return len;
    }
    if (is_eof && buffer_write > 0)
    {
        memcpy(buf, buffer + buffer_pos, buffer_write);
        buffer_pos += buffer_write;
        buffer_write = 0;
        return len;
    }

    return 0;
}

uint32_t StreamHttp::read(uint8_t *buffer, uint32_t len)
{
    if (buffer_write == 0 && get_less_read() == 0)
    {
        return 0;
    }
    uint32_t size = buffer_read(buffer, len);
    if (size)
    {
        return size;
    }
    read_block();

    return buffer_read(buffer, len);
}

uint32_t StreamHttp::write(uint8_t *buffer, uint32_t len)
{
    return 0;
}

uint32_t StreamHttp::buffer_peek(uint8_t *buf, uint32_t len)
{
    if (buffer_write >= len)
    {
        memcpy(buf, buffer + buffer_pos, len);
        return len;
    }
    if (is_eof && buffer_write > 0)
    {
        memcpy(buf, buffer + buffer_pos, buffer_write);
        return len;
    }

    return 0;
}

uint32_t StreamHttp::peek(uint8_t *buffer, uint32_t len)
{
    if (buffer_write == 0 && get_less_read() == 0)
    {
        return 0;
    }
    uint32_t size = buffer_peek(buffer, len);
    if (size)
    {
        return size;
    }
    read_block();

    return buffer_peek(buffer, len);
}

uint32_t StreamHttp::get_pos()
{
    return http_pos - buffer_write;
}

uint32_t StreamHttp::get_all_size()
{
    return http_size;
}

uint32_t StreamHttp::get_less_read()
{
    if (http_size > 0)
    {
        return http_size - get_pos();
    }

    return 0;
}

void StreamHttp::re_connect(uint32_t pos)
{
    http_pos = http->re_connect(get_pos() + pos);
    buffer_pos = 0;
    buffer_write = 0;
}

void StreamHttp::seek(int32_t pos, uint8_t where)
{
    if (where == SEEK_SET)
    {
        uint32_t res = http->re_connect(pos);
        http_pos = res;
    }
    else if (where == SEEK_CUR)
    {
        if (pos < 0)
        {
            if (buffer_pos > 0)
            {
                if (buffer_pos + pos < 0)
                {
                    re_connect(pos);
                }
                else
                {
                    buffer_pos += pos;
                    buffer_write -= pos;
                }
            }
            else
            {
                re_connect(pos);
            }
        }
        else if (pos >= 0)
        {
            if (pos > buffer_write)
            {
                for (;;)
                {
                    if (pos >= buffer_size)
                    {
                        buffer_write = 0;
                        buffer_pos = 0;
                        read_block();
                        if (buffer_write == 0)
                        {
                            buffer_pos = 0;
                            buffer_write = 0;
                            return;
                        }
                        pos -= buffer_write;
                    }
                    else if (pos >= buffer_write)
                    {
                        read_block();
                        if (buffer_write < pos)
                        {
                            buffer_pos = 0;
                            buffer_write = 0;
                            return;
                        }
                        buffer_pos += pos;
                        buffer_write -= pos;
                        break;
                    }
                    else
                    {
                        buffer_pos += pos;
                        buffer_write -= pos;
                        break;
                    }
                }
            }
            else
            {
                buffer_pos += pos;
                buffer_write -= pos;
            }
        }
    }
    else if (where == SEEK_END)
    {
    }
}

bool StreamHttp::test_read_size(uint32_t size)
{
    if (buffer_write == 0 && get_less_read() == 0)
    {
        return 0;
    }
    if (size > buffer_write)
    {
        read_block();
        return buffer_write >= size;
    }

    return true;
}

bool StreamHttp::can_read()
{
    if (http_size == 0)
    {
        return true;
    }
    return http_pos < http_size || buffer_write > 0;
}