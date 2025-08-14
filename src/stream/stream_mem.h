#ifndef _STREAM_MEM_H_
#define _STREAM_MEM_H_

#include "common/data_item.h"

#include "stream.h"

namespace coloraudio::stream
{
    class MemoryStream : public BaseStream
    {
    private:
        uint8_t *buffer_mem;
        uint32_t buffer_pos = 0;
        uint32_t buffer_size;

    public:
        MemoryStream(uint8_t *buffer, uint32_t size);
        MemoryStream(coloraudio::common::DataItem *item);
        ~MemoryStream();

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
#endif
