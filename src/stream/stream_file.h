#ifndef _STREAM_FILE_H_
#define _STREAM_FILE_H_

#include "stream.h"

#include <stdio.h>
#include <string>

namespace ColorAudio
{
    class StreamFile : public Stream
    {
    private:
        uint32_t size;
        uint32_t pos;
        FILE *file;
        char *path;

    public:
        StreamFile(const char *path);
        StreamFile(std::string stp) : StreamFile(stp.c_str()) {}
        ~StreamFile();

        uint32_t read(uint8_t *buffer, uint32_t len);
        uint32_t write(uint8_t *buffer, uint32_t len);
        uint32_t peek(uint8_t *buffer, uint32_t len);
        uint32_t get_pos()
        {
            return this->pos;
        }
        uint32_t get_all_size()
        {
            return this->size;
        }
        uint32_t get_less_read()
        {
            return (this->size > this->pos) ? (this->size - this->pos) : 0;
        }

        void seek(int32_t pos, uint8_t where);

        bool test_read_size(uint32_t size)
        {
            return this->pos + size <= this->size;
        }
        bool can_read()
        {
            return this->pos < this->size;
        }

        StreamFile *copy();
    };
}

#endif