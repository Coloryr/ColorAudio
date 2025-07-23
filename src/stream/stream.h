#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    STREAM_TYPE_FILE = 0,
    STREAM_TYPE_MEM,
    STREAM_TYPE_CIR,
    STREAM_TYPE_HTTP,
    STREAM_TYPE_NCM,
    STREAM_TYPE_UNKNOW = -1
} steam_type;

constexpr auto STREAM_BUFFER_SIZE = 1024 * 8;

namespace ColorAudio
{
    class Stream
    {
    private:
        steam_type type;

    public:
        Stream(steam_type type);
        virtual ~Stream();

        virtual uint32_t read(uint8_t *buffer, uint32_t len) = 0;
        virtual uint32_t write(uint8_t *buffer, uint32_t len) = 0;
        virtual uint32_t peek(uint8_t *buffer, uint32_t len) = 0;
        virtual uint32_t get_pos() = 0;
        virtual uint32_t get_all_size() = 0;
        virtual uint32_t get_less_read() = 0;

        virtual void seek(int32_t pos, uint8_t where) = 0;

        virtual bool test_read_size(uint32_t size) = 0;
        virtual bool can_read() = 0;

        uint8_t read_byte();
    };
}

#endif