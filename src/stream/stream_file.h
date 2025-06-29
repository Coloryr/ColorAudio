#ifndef _STREAM_FILE_H_
#define _STREAM_FILE_H_

#include "stream.h"

class StreamFile : Stream
{
private:
    uint32_t size;
    uint32_t pos;
    FILE* file;
    char* path;

public:
    StreamFile(char* path);
    ~StreamFile();

    uint32_t read(uint8_t* buffer, uint32_t len);
    uint32_t write(uint8_t* buffer, uint32_t len);
    uint32_t peek(uint8_t* buffer, uint32_t len);
    uint32_t get_pos();
    uint32_t get_all_size();
    uint32_t get_less_read();

    void seek(int32_t pos, uint8_t where);

    bool test_read_size(uint32_t size);
    bool can_read();

    StreamFile* copy();
};

#endif