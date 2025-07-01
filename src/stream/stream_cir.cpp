#include "stream_cir.h"

#include <malloc.h>
#include <pthread.h>
#include <string.h>

using namespace ColorAudio;

StreamCir::StreamCir() : Stream(STREAM_TYPE_CIR)
{
    this->buffer = static_cast<uint8_t*>(malloc(STREAM_BUFFER_SIZE));
    if (!this->buffer)
    {
        return;
    }

    this->capacity = STREAM_BUFFER_SIZE;
    this->read_pos = 0;
    this->write_pos = 0;
    this->bytes_avail = 0;

    pthread_mutex_init(&this->mutex, NULL);
    pthread_cond_init(&this->not_empty, NULL);
    pthread_cond_init(&this->not_full, NULL);
}

StreamCir::~StreamCir()
{
    pthread_mutex_lock(&this->mutex);

    pthread_cond_broadcast(&this->not_empty);
    pthread_cond_broadcast(&this->not_full);
    pthread_mutex_unlock(&this->mutex);

    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->not_empty);
    pthread_cond_destroy(&this->not_full);

    if (this->buffer)
    {
        free(this->buffer);
    }
}

uint32_t StreamCir::read(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&this->mutex);

    while (this->bytes_avail == 0)
    {
        pthread_cond_wait(&this->not_empty, &this->mutex);
    }

    if (this->bytes_avail == 0)
    {
        pthread_mutex_unlock(&this->mutex);
        return 0;
    }

    uint32_t bytes_to_read = (len > this->bytes_avail) ? this->bytes_avail : len;

    uint32_t first_chunk = this->capacity - this->read_pos;
    if (bytes_to_read <= first_chunk)
    {
        memcpy(buffer, this->buffer + this->read_pos, bytes_to_read);
        this->read_pos = (this->read_pos + bytes_to_read) % this->capacity;
    }
    else
    {
        memcpy(buffer, this->buffer + this->read_pos, first_chunk);
        memcpy(buffer + first_chunk, this->buffer, static_cast<size_t>(bytes_to_read) - first_chunk);
        this->read_pos = bytes_to_read - first_chunk;
    }

    this->bytes_avail -= bytes_to_read;

    pthread_cond_signal(&this->not_full);
    pthread_mutex_unlock(&this->mutex);

    return bytes_to_read;
}

uint32_t StreamCir::write(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&this->mutex);

    while (this->capacity - this->bytes_avail < len)
    {
        pthread_cond_wait(&this->not_full, &this->mutex);
    }

    uint32_t free_space = this->capacity - this->bytes_avail;
    uint32_t bytes_to_write = (len > free_space) ? free_space : len;

    uint32_t first_chunk = this->capacity - this->write_pos;
    if (bytes_to_write <= first_chunk)
    {
        memcpy(this->buffer + this->write_pos, buffer, bytes_to_write);
        this->write_pos = (this->write_pos + bytes_to_write) % this->capacity;
    }
    else
    {
        memcpy(this->buffer + this->write_pos, buffer, first_chunk);
        memcpy(this->buffer, buffer + first_chunk, static_cast<size_t>(bytes_to_write) - first_chunk);
        this->write_pos = bytes_to_write - first_chunk;
    }

    this->bytes_avail += bytes_to_write;

    pthread_cond_signal(&this->not_empty);
    pthread_mutex_unlock(&this->mutex);

    return bytes_to_write;
}

uint32_t StreamCir::peek(uint8_t* buffer, uint32_t len)
{
    pthread_mutex_lock(&this->mutex);

    while (this->bytes_avail == 0)
    {
        pthread_cond_wait(&this->not_empty, &this->mutex);
    }

    if (this->bytes_avail == 0)
    {
        pthread_mutex_unlock(&this->mutex);
        return 0;
    }

    uint32_t bytes_to_peek = (len > this->bytes_avail) ? this->bytes_avail : len;
    uint32_t temp_read_pos = this->read_pos;

    uint32_t first_chunk = this->capacity - temp_read_pos;
    if (bytes_to_peek <= first_chunk)
    {
        memcpy(buffer, this->buffer + temp_read_pos, bytes_to_peek);
    }
    else
    {
        memcpy(buffer, this->buffer + temp_read_pos, first_chunk);
        memcpy(buffer + first_chunk, this->buffer, static_cast<size_t>(bytes_to_peek) - first_chunk);
    }

    pthread_mutex_unlock(&this->mutex);

    return bytes_to_peek;
}

uint32_t StreamCir::get_pos()
{
    pthread_mutex_lock(&this->mutex);
    uint32_t pos = this->read_pos;
    pthread_mutex_unlock(&this->mutex);
    return pos;
}

uint32_t StreamCir::get_all_size()
{
    return this->capacity;
}
uint32_t StreamCir::get_less_read()
{
    pthread_mutex_lock(&this->mutex);
    uint32_t avail = this->bytes_avail;
    pthread_mutex_unlock(&this->mutex);
    return avail;
}

void StreamCir::seek(int32_t pos, uint8_t where)
{
    pthread_mutex_lock(&this->mutex);

    switch (where)
    {
    case SEEK_SET:
        if (pos >= 0 && (uint32_t)pos <= this->bytes_avail)
        {
            this->read_pos = (this->write_pos - this->bytes_avail + pos) % this->capacity;
            this->bytes_avail -= pos;
        }
        break;

    case SEEK_CUR:
        if (pos > 0)
        {
            uint32_t forward = (pos > this->bytes_avail) ? this->bytes_avail : pos;
            this->read_pos = (this->read_pos + forward) % this->capacity;
            this->bytes_avail -= forward;
        }
        else if (pos < 0)
        {
            
        }
        break;

    case SEEK_END:
        if (pos <= 0 && (uint32_t)(-pos) <= this->bytes_avail)
        {
            this->bytes_avail += pos;
        }
        break;
    }

    pthread_mutex_unlock(&this->mutex);
}

bool StreamCir::test_read_size(uint32_t size)
{
    pthread_mutex_lock(&this->mutex);
    bool ret = (this->bytes_avail >= size);
    pthread_mutex_unlock(&this->mutex);
    return ret;
}
bool StreamCir::can_read()
{
    return true;
}