#include "stream.h"
#include "utils.h"

#include "lvgl/src/misc/lv_log.h"

#include <malloc.h>
#include <string.h>

stream *stream_open_file(char *path)
{
    if (path == NULL)
    {
        LV_LOG_ERROR("[stream] Can't open null file");
        return NULL;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        LV_LOG_ERROR("[stream] Can't open file: %s", path);
        return NULL;
    }

    file_stream *st = malloc(sizeof(file_stream));
    uint16_t len = get_length(path);
    st->path = malloc(len + 1);
    st->buffer = malloc(STREAM_BUFFER_SIZE);
    st->buffer_pos = 0;
    st->buffer_size = STREAM_BUFFER_SIZE;
    memcpy(st->path, path, len);
    st->path[len] = 0;
    st->file = file;
    st->pos = 0;

    uint32_t fsta = ftell(file);
    fseek(file, 0, SEEK_END);
    uint32_t fend = ftell(file);
    uint32_t flen = fend - fsta;
    if (flen > 0)
        fseek(file, 0, SEEK_SET);

    st->size = flen;
    st->type = STREAM_TYPE_FILE;

    return (stream *)st;
}

void stream_close(stream *stream)
{
    if (stream == NULL || stream->type == STREAM_TYPE_UNKNOW)
    {
        return;
    }

    if (stream->buffer)
    {
        free(stream->buffer);
    }

    if (stream->type == STREAM_TYPE_FILE)
    {
        file_stream *stream1 = (file_stream *)stream;
        fclose(stream1->file);
        if (stream1->path)
        {
            free(stream1->path);
        }
    }

    free(stream);
}

uint32_t stream_read(stream *stream, void *buffer, uint32_t len)
{
    if (stream == NULL || stream->type == STREAM_TYPE_UNKNOW)
    {
        return 0;
    }

    if (stream->type == STREAM_TYPE_FILE)
    {
        file_stream *stream1 = (file_stream *)stream;
        uint32_t temp = fread(buffer, 1, len, stream1->file);
        stream->pos += temp;
        return temp;
    }

    return 0;
}

uint8_t stream_read_byte(stream *stream)
{
    if (stream == NULL || stream->type == STREAM_TYPE_UNKNOW)
    {
        return 0;
    }

    if (stream->type == STREAM_TYPE_FILE)
    {
        uint8_t buffer;
        file_stream *stream1 = (file_stream *)stream;
        uint32_t temp = fread(&buffer, 1, 1, stream1->file);
        stream->pos += temp;
        return buffer;
    }

    return 0;
}

void stream_seek(stream *stream, int32_t pos, uint8_t where)
{
    if (stream == NULL || stream->type == STREAM_TYPE_UNKNOW)
    {
        return;
    }

    if (stream->type == STREAM_TYPE_FILE)
    {
        file_stream *stream1 = (file_stream *)stream;
        fseek(stream1->file, pos, where);
        stream1->pos = ftell(stream1->file);
    }
}

stream *stream_copy_file(stream *st)
{
    if (st->type == STREAM_TYPE_FILE)
    {
        file_stream *stream1 = (file_stream *)st;
        stream *st1 = stream_open_file(stream1->path);
        if(st1 == NULL)
        {
            return NULL;
        }
        stream_seek(st1, st->pos, SEEK_SET);
        return st1;
    }
    return NULL;
}