#include "stream_http.h"

#include <malloc.h>

StreamHttp::StreamHttp(IStreamHttp* http) : Stream(STREAM_TYPE_HTTP),
    http(http)
{
    cir = new StreamCir();
    uint8_t* buffer = static_cast<uint8_t*>(malloc(STREAM_BUFFER_SIZE));
    if (buffer == nullptr)
    {
        delete cir;
        return;
    }

    http_size = http->get_size();
    http_pos = 0;
}

StreamHttp::~StreamHttp()
{
    if (this->cir)
    {
        delete this->cir;
    }

    this->http->close();
}

uint32_t StreamHttp::read(uint8_t* buffer, uint32_t len)
{
    if (cir->get_read_pos() >= len)
    {
        return cir->read(buffer, len);
    }
    uint32_t need = len - cir->get_read_pos();
    uint32_t rsize = http->read(buffer, need);
    cir->write(buffer, rsize);

    return cir->read(buffer, len);
}

uint32_t StreamHttp::write(uint8_t* buffer, uint32_t len)
{
    //²»Ö§³Ö
    return 0;
}

uint32_t StreamHttp::peek(uint8_t* buffer, uint32_t len)
{
    if (cir->get_read_pos() >= len)
    {
        return cir->peek(buffer, len);
    }
    uint32_t need = len - cir->get_read_pos();
    uint32_t rsize = http->read(buffer, need);
    http_pos += rsize;
    cir->write(buffer, rsize);

    return cir->peek(buffer, len);
}

uint32_t StreamHttp::get_pos()
{
    return http_pos - cir->get_read_avail();
}

uint32_t StreamHttp::get_all_size()
{
    return http_size;
}

uint32_t StreamHttp::get_less_read()
{
    if (http_size > 0)
    {
        return http_size - http_pos + cir->get_read_avail();
    }
}

void StreamHttp::seek(int32_t pos, uint8_t where)
{
    if (where == SEEK_SET)
    {
        cir->reset();
        uint32_t pos = http->re_connect(pos);
        http_pos = pos;
    }
    else if (where == SEEK_CUR)
    {
        if (cir->get_read_pos() >= pos)
        {
            cir->seek(pos, SEEK_CUR);
        }
        else
        {
            cir->reset();

            http->re_connect(pos);
        }
        http_pos += pos;
    }
}

bool StreamHttp::test_read_size(uint32_t size)
{
    bool ret;
    if (cir->get_read_pos() >= size)
    {
        ret = (cir->get_read_avail() >= size);
        return ret;
    }

    uint32_t need = size - cir->get_read_pos();
    uint32_t rsize = http->read(buffer, need);
    http_pos += rsize;
    cir->write(buffer, rsize);

    return (cir->get_read_avail() >= size);
}

bool StreamHttp::can_read()
{
    if (http_size == 0)
    {
        return true;
    }
    return http_pos < http_size;
}