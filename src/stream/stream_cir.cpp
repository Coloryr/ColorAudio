#include "stream_cir.h"

#include <malloc.h>
#include <pthread.h>
#include <string.h>

using namespace ColorAudio;

StreamCir::StreamCir() : Stream(STREAM_TYPE_CIR)
{
    buffer = static_cast<uint8_t*>(malloc(STREAM_BUFFER_SIZE));
    if (!buffer)
    {
        return;
    }

    capacity = STREAM_BUFFER_SIZE;
    read_pos = 0;
    write_pos = 0;
    bytes_avail = 0;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);
}

StreamCir::~StreamCir()
{
    pthread_mutex_lock(&mutex);

    pthread_cond_broadcast(&not_empty);
    pthread_cond_broadcast(&not_full);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    if (buffer)
    {
        free(buffer);
    }
}

uint32_t StreamCir::read(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&mutex);

    while (bytes_avail == 0)
    {
        pthread_cond_wait(&not_empty, &mutex);
    }

    if (bytes_avail == 0)
    {
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    uint32_t bytes_to_read = (len > bytes_avail) ? bytes_avail : len;

    uint32_t first_chunk = capacity - read_pos;
    if (bytes_to_read <= first_chunk)
    {
        memcpy(buffer, buffer + read_pos, bytes_to_read);
        read_pos = (read_pos + bytes_to_read) % capacity;
    }
    else
    {
        memcpy(buffer, buffer + read_pos, first_chunk);
        memcpy(buffer + first_chunk, buffer, static_cast<size_t>(bytes_to_read) - first_chunk);
        read_pos = bytes_to_read - first_chunk;
    }

    bytes_avail -= bytes_to_read;

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);

    return bytes_to_read;
}

uint32_t StreamCir::write(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&mutex);

    while (capacity - bytes_avail < len)
    {
        pthread_cond_wait(&not_full, &mutex);
    }

    uint32_t free_space = capacity - bytes_avail;
    uint32_t bytes_to_write = (len > free_space) ? free_space : len;

    uint32_t first_chunk = capacity - write_pos;
    if (bytes_to_write <= first_chunk)
    {
        memcpy(buffer + write_pos, buffer, bytes_to_write);
        write_pos = (write_pos + bytes_to_write) % capacity;
    }
    else
    {
        memcpy(buffer + write_pos, buffer, first_chunk);
        memcpy(buffer, buffer + first_chunk, static_cast<size_t>(bytes_to_write) - first_chunk);
        write_pos = bytes_to_write - first_chunk;
    }

    bytes_avail += bytes_to_write;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);

    return bytes_to_write;
}

uint32_t StreamCir::peek(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&mutex);

    while (bytes_avail == 0)
    {
        pthread_cond_wait(&not_empty, &mutex);
    }

    if (bytes_avail == 0)
    {
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    uint32_t bytes_to_peek = (len > bytes_avail) ? bytes_avail : len;
    uint32_t temp_read_pos = read_pos;

    uint32_t first_chunk = capacity - temp_read_pos;
    if (bytes_to_peek <= first_chunk)
    {
        memcpy(buffer, buffer + temp_read_pos, bytes_to_peek);
    }
    else
    {
        memcpy(buffer, buffer + temp_read_pos, first_chunk);
        memcpy(buffer + first_chunk, buffer, static_cast<size_t>(bytes_to_peek) - first_chunk);
    }

    pthread_mutex_unlock(&mutex);

    return bytes_to_peek;
}

uint32_t StreamCir::get_pos()
{
    pthread_mutex_lock(&mutex);
    uint32_t pos = read_pos;
    pthread_mutex_unlock(&mutex);
    return pos;
}

uint32_t StreamCir::get_all_size()
{
    return capacity;
}
uint32_t StreamCir::get_less_read()
{
    pthread_mutex_lock(&mutex);
    uint32_t avail = bytes_avail;
    pthread_mutex_unlock(&mutex);
    return avail;
}

void StreamCir::seek(int32_t pos, uint8_t where)
{
    pthread_mutex_lock(&mutex);

    switch (where)
    {
    case SEEK_SET:
        if (pos >= 0 && (uint32_t)pos <= bytes_avail)
        {
            read_pos = (write_pos - bytes_avail + pos) % capacity;
            bytes_avail -= pos;
        }
        break;

    case SEEK_CUR:
        if (pos > 0)
        {
            uint32_t forward = (pos > bytes_avail) ? bytes_avail : pos;
            read_pos = (read_pos + forward) % capacity;
            bytes_avail -= forward;
        }
        else if (pos < 0)
        {
            
        }
        break;

    case SEEK_END:
        if (pos <= 0 && (uint32_t)(-pos) <= bytes_avail)
        {
            bytes_avail += pos;
        }
        break;
    }

    pthread_mutex_unlock(&mutex);
}

bool StreamCir::test_read_size(uint32_t size)
{
    pthread_mutex_lock(&mutex);
    bool ret = bytes_avail >= size;
    pthread_mutex_unlock(&mutex);
    return ret;
}
bool StreamCir::can_read()
{
    return true;
}