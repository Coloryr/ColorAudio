#ifndef _STREAM_CIR_H_
#define _STREAM_CIR_H_

#include "stream.h"

#include <pthread.h>

class StreamCir : Stream
{
private:
    uint8_t* buffer;      // 缓冲区内存指针
    uint32_t capacity;    // 缓冲区总容量
    uint32_t read_pos;    // 读位置指针
    uint32_t write_pos;   // 写位置指针
    uint32_t bytes_avail; // 可读字节数

    pthread_mutex_t mutex;    // 互斥锁
    pthread_cond_t not_empty; // 非空条件变量
    pthread_cond_t not_full;  // 非满条件变量

public:
    StreamCir();
    ~StreamCir();

    uint32_t read(uint8_t* buffer, uint32_t len);
    uint32_t write(uint8_t* buffer, uint32_t len);
    uint32_t peek(uint8_t* buffer, uint32_t len);
    uint32_t get_pos();
    uint32_t get_all_size();
    uint32_t get_less_read();
    uint8_t read_byte();

    void seek(int32_t pos, uint8_t where);

    bool test_read_size(uint32_t size);
    bool can_read();

    uint32_t get_read_pos() const
    {
        return read_pos;
    }

    void reset() 
    {
        read_pos = 0;
        write_pos = 0;
    }

    uint32_t get_read_avail() const
    {
        return bytes_avail;
    }
};

#endif // !_STREAM_CIR_H_
