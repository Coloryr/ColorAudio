#include "mp3id3.h"
#include "stream.h"
#include "utils.h"

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

static void mp3_id3_cov_str(stream *st, mp3id3_tag *tag)
{
    uint8_t type = stream_read_byte(st);
    uint8_t *temp;
    tag->size -= 1;
    switch (type)
    {
    case 0:
        // temp = malloc(tag->size);
        stream_seek(st, tag->size, SEEK_SET);
        // free(temp);
        break;
    case 1:
        temp = malloc(tag->size);
        stream_read(st, temp, tag->size);
        tag->size = utf16_to_utf8((uint16_t *)temp, &tag->data, tag->size);
        free(temp);
        break;
    case 2:
        tag->data = malloc(tag->size);
        stream_read(st, tag->data, tag->size);
        break;
    default:
        break;
    }
}

mp3id3 *mp3_id3_read(stream *st)
{
    uint8_t buffer[16];
    stream_read(st, buffer, 3);
    if (buffer[0] != 'I' || buffer[1] != 'D' || buffer[2] != '3')
    {
        return NULL;
    }
    mp3id3 *id3 = malloc(sizeof(mp3id3));
    memset(id3, 0, sizeof(mp3id3));

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
            mp3_id3_cov_str(st, &id3->title);
            break;
        case ID3_AUTHER_TAG:
            id3->auther.size = size;
            mp3_id3_cov_str(st, &id3->auther);
            break;
        case ID3_ALBUM_TAG:
            id3->album.size = size;
            mp3_id3_cov_str(st, &id3->album);
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