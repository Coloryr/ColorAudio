#ifndef _STREAM_H_
#define _STREAM_H_

#include "stdint.h"
#include <stdio.h>

typedef enum {
    STREAM_TYPE_FILE = 0,
    STREAM_TYPE_MEM,
    STREAM_TYPE_UNKNOW = -1
} steam_type;

#define STREAM_BUFFER_SIZE 8192

#define BASE_STREAM \
    uint32_t size; \
    uint32_t pos; \
    steam_type type; \
    uint8_t* buffer; \
    uint32_t buffer_pos; \
    uint32_t buffer_size;

typedef struct
{
    BASE_STREAM
} stream;

typedef struct 
{
    BASE_STREAM

    FILE* file;
    uint8_t* path;
} file_stream;

stream *stream_open_file(char *path);
void stream_close(stream *stream);
uint32_t stream_read(stream *stream, void *buffer, uint32_t len);
uint8_t stream_read_byte(stream *stream);
void stream_seek(stream *stream, int32_t pos, uint8_t where);
stream *stream_copy_file(stream *stream);

#endif