#ifndef _STREAM_HTTP_H_
#define _STREAM_HTTP_H_

#include "stream.h"
#include "stream_cir.h"

namespace ColorAudio
{
    class IStreamHttp
    {
    public:
        virtual void close() = 0;
        virtual uint32_t get_size() = 0;
        virtual uint32_t read(uint8_t *buffer, uint32_t len) = 0;
        virtual uint32_t re_connect(uint32_t pos) = 0;
    };

    class StreamHttp : public Stream
    {
    private:
        uint8_t *buffer;

        IStreamHttp *http;
        uint32_t buffer_size;
        uint32_t buffer_pos;
        uint32_t buffer_write;
        uint32_t http_size;
        uint32_t http_pos;

        bool is_eof;

        void read_block();

        uint32_t buffer_read(uint8_t *buf, uint32_t len);
        uint32_t buffer_peek(uint8_t *buf, uint32_t len);
        void re_connect(uint32_t pos);

    public:
        StreamHttp(IStreamHttp *http);
        ~StreamHttp();

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

#endif // !_STREAM_HTTP_H_
