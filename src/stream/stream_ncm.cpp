#include "stream_ncm.h"

#include "../common/utils.h"

#include "malloc.h"

using namespace ColorAudio;

StreamNcm::StreamNcm(Stream *st1, NeteaseCrypt *cy) : Stream(STREAM_TYPE_NCM)
{
    buffer = static_cast<uint8_t *>(malloc(BLOCK_SIZE));
    buffer_size = BLOCK_SIZE;
    buffer_pos = 0;
    buffer_write = 0;
    st = st1;
    cry = cy;
    mark = st1->get_pos();
}

StreamNcm::~StreamNcm()
{
    if (buffer)
    {
        free(buffer);
    }
    if (st)
    {
        delete st;
    }
    if (cry)
    {
        delete cry;
    }
}

void StreamNcm::read_block()
{
    uint32_t size = st->read(buffer, BLOCK_SIZE);
    cry->Dump(buffer, BLOCK_SIZE);

    buffer_pos = 0;
    buffer_write = size;
}

uint32_t StreamNcm::read(uint8_t *buf, uint32_t len)
{
    uint32_t read = 0;
    for (;;)
    {
        int32_t have = buffer_write - buffer_pos;
        if (have <= 0)
        {
            read_block();
            if (buffer_write == 0)
            {
                return read;
            }
            have = buffer_write - buffer_pos;
        }

        if (len > have)
        {
            memcpy(buf + read, buffer + buffer_pos, have);
            buffer_pos += have;
            len -= have;
            read += have;
        }
        else
        {
            memcpy(buf + read, buffer + buffer_pos, len);
            buffer_pos += len;
            read += len;
            return read;
        }
    }
}
uint32_t StreamNcm::write(uint8_t *buf, uint32_t len)
{
    return st->write(buf, len);
}
uint32_t StreamNcm::peek(uint8_t *buf, uint32_t len)
{
    uint32_t size = st->peek(buf, len);
    cry->Dump(buf, len);

    return size;
}
uint32_t StreamNcm::get_pos()
{
    return st->get_pos() - mark - buffer_write + buffer_pos;
}
uint32_t StreamNcm::get_all_size()
{
    return st->get_all_size() - mark;
}
uint32_t StreamNcm::get_less_read()
{
    return st->get_less_read();
}

void StreamNcm::seek(int32_t pos, uint8_t where)
{
    if (where == SEEK_CUR)
    {
        if (pos > 0)
        {
            uint32_t have = buffer_write - buffer_pos;
            if (have >= pos)
            {
                buffer_pos += pos;
            }
            else
            {
                pos -= have;
                uint32_t block = pos / buffer_size;
                if (block > 0)
                {
                    uint32_t down = block * buffer_size;
                    st->seek(down, SEEK_CUR);
                    pos -= down;
                }

                read_block();
                buffer_pos = pos;
            }
        }
        else
        {
            if (buffer_pos + pos > 0)
            {
                buffer_pos += pos;
            }
            else
            {
                uint32_t block = -pos / buffer_size;
                if (block > 0)
                {
                    uint32_t down = block * buffer_size;
                    st->seek(down, SEEK_CUR);
                    pos -= down;
                }
            }
        }
    }
    else if (where == SEEK_SET)
    {
        uint32_t block = pos / buffer_size;
        if (block > 0)
        {
            uint32_t down = block * buffer_size;
            st->seek(mark + down, SEEK_SET);
            pos -= down;
        }
        else
        {
            st->seek(mark, SEEK_SET);
        }

        read_block();
        buffer_pos = pos;
    }
}

bool StreamNcm::test_read_size(uint32_t size)
{
    return st->test_read_size(size);
}
bool StreamNcm::can_read()
{
    return st->can_read();
}