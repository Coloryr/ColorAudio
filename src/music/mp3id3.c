#include "mp3id3.h"
#include "stream.h"

#include <malloc.h>
#include <string.h>

static id3_type test_id3_type(uint8_t *buffer)
{
    if (strncmp(buffer, TITLE_TAG, 4) == 0)
    {
        return ID3_TITLE_TAG;
    }
    else if (strncmp(buffer, AUTHER_TAG, 4) == 0)
    {
        return ID3_AUTHER_TAG;
    }
    else if (strncmp(buffer, ALBUM_TAG, 4) == 0)
    {
        return ID3_ALBUM_TAG;
    }
    else if (strncmp(buffer, TRACK_TAG, 4) == 0)
    {
        return ID3_TRACK_TAG;
    }
    else if (strncmp(buffer, TIME_TAG, 4) == 0)
    {
        return ID3_TIME_TAG;
    }
    else if (strncmp(buffer, TCON_TAG, 4) == 0)
    {
        return ID3_TCON_TAG;
    }
    else if (strncmp(buffer, COMMENT_TAG, 4) == 0)
    {
        return ID3_COMMENT_TAG;
    }
    else if (strncmp(buffer, PICTURE_TAG, 4) == 0)
    {
        return ID3_PICTURE_TAG;
    }

    return ID3_UNKNOW_TAG;
}

static uint32_t id3_skip(stream *st)
{
    uint32_t size = 0;
    uint8_t b;
    while ((b = stream_read_byte(st)) != 0)
    {
        if (b == -1)
            break;
        size++;
    }

    return size;
}

mp3id3 *mp3_id3_read(stream *st)
{
    mp3id3 *id3 = malloc(sizeof(mp3id3));
    memset(id3, 0, sizeof(mp3id3));

    uint8_t buffer[16];
    stream_read(st, buffer, 7);
    id3->version = buffer[0];
    id3->r_version = buffer[1];
    id3->flag = buffer[2];
    id3->length = (buffer[3] << 21) + (buffer[4] << 14) + (buffer[5] << 7) + buffer[6];
    uint32_t len = id3->length;
    uint32_t pos = 0;

    do
    {
        stream_read(st, buffer, 4);
        if (buffer[0] == 0)
        {
            stream_seek(st, len + 10, SEEK_SET);
            return id3;
        }
        pos += 4;
        id3_type type = test_id3_type(buffer);
        stream_read(st, buffer, 6);
        pos += 6;
        uint32_t size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
        switch (type)
        {
        case ID3_TITLE_TAG:
            id3->title.size = size;
            id3->title.data = malloc(size);
            stream_read(st, id3->title.data, size);
            break;
        case ID3_AUTHER_TAG:
            id3->auther.size = size;
            id3->auther.data = malloc(size);
            stream_read(st, id3->auther.data, size);
            break;
        case ID3_ALBUM_TAG:
            id3->album.size = size;
            id3->album.data = malloc(size);
            stream_read(st, id3->album.data, size);
            break;
        case ID3_PICTURE_TAG:
            uint8_t encoding = stream_read_byte(st);
            uint32_t mimeType = id3_skip(st);
            uint32_t description = id3_skip(st);
            uint32_t imageSize = size - (1 + mimeType + 1 + 1 + description + 1);
            id3->image.size = size;
            id3->image.data = malloc(size);
            stream_read(st, id3->image.data, size);
            break;
        default:
            stream_seek(st, size, SEEK_CUR);
            break;
        }
        pos += size;
    } while (pos < len);

    return id3;
}

void mp3_id3_close(mp3id3 *id3)
{
    if (id3 == NULL)
    {
        return;
    }
    if (id3->title.data)
    {
        free(id3->title.data);
    }
    if (id3->auther.data)
    {
        free(id3->auther.data);
    }
    if (id3->album.data)
    {
        free(id3->album.data);
    }
    if (id3->image.data)
    {
        free(id3->image.data);
    }

    free(id3);
}