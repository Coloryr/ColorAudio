#ifndef __STREAM_NCM_H__
#define __STREAM_NCM_H__

#include "stream.h"

#include "ncmcrypt.h"

#define BLOCK_SIZE 0x8000

namespace ColorAudio
{
    class StreamNcm : public Stream
    {
    private:
        uint8_t* buffer;
        uint32_t buffer_size;
        uint32_t buffer_pos;
        uint32_t buffer_write;

        Stream *st;
        NeteaseCrypt *cry;
        uint32_t mark;

        void read_block();

    public:
        StreamNcm(Stream *st1, NeteaseCrypt *cy);
        ~StreamNcm();

        uint32_t read(uint8_t *buffer, uint32_t len);
        uint32_t write(uint8_t *buffer, uint32_t len);
        uint32_t peek(uint8_t *buffer, uint32_t len);
        uint32_t get_pos();
        uint32_t get_all_size();
        uint32_t get_less_read();

        void seek(int32_t pos, uint8_t where);

        bool test_read_size(uint32_t size);
        bool can_read();
    };
}

#endif // __STREAM_NCM_H__