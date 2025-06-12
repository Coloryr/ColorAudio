#ifndef _STREAM_H_
#define _STREAM_H_

#include "stdint.h"
#include <stdio.h>

typedef enum {
    STREAM_TYPE_FILE = 0,
    STREAM_TYPE_UNKNOW = -1
} steam_type;

typedef struct
{
    uint32_t size;
    uint32_t pos;
    steam_type type;
} stream;

typedef struct 
{
    uint32_t size;
    uint32_t pos;
    steam_type type;
    FILE* file;
} file_stream;

stream *stream_open_file(char *path);
void stream_close(stream *stream);
uint32_t stream_read(stream *stream, void *buffer, uint32_t len);
uint8_t stream_read_byte(stream *stream);
void stream_seek(stream *stream, uint32_t pos, uint8_t where);

#endif