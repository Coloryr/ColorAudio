#include "mp3_id3.h"

#include "../common/utils.h"

#include <malloc.h>
#include <string.h>
#include <string>

using namespace ColorAudio;

/**
 * 判断MP3ID3帧类型
 * @param buffer 需要判断的数据
 * @return id3帧类型
 */
static id3_type test_id3_type(char *buffer)
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

/**
 * 一直读取数据，直到是0数据
 */
static uint32_t skip_data(ColorAudio::Stream *st)
{
    uint32_t size = 0;
    uint8_t b;
    while ((b = st->read_byte()) != 0)
    {
        if (b == -1)
            break;
        size++;
    }

    return size;
}

static void cov_str(ColorAudio::Stream *st, char **tag, uint32_t size)
{
    *tag = nullptr;
    uint8_t type = st->read_byte();
    size -= 1;
    switch (type)
    {
    case 0:
    {
        uint8_t buffer[4];
        st->read(buffer, 4);
        size -= 4;
        *tag = static_cast<char *>(malloc(size));
        st->read(reinterpret_cast<uint8_t *>(*tag), size);
        break;
    }
    case 1:
    {
        uint8_t *temp = static_cast<uint8_t *>(malloc(size));
        st->read(temp, size);
        utf16_to_utf8(reinterpret_cast<uint16_t *>(temp), tag, size);
        free(temp);
        break;
    }
    case 2:
        *tag = static_cast<char *>(malloc(size));
        st->read(reinterpret_cast<uint8_t *>(*tag), size);
        break;
    default:
        break;
    }
}

bool mp3id3_have(ColorAudio::Stream *st)
{
    uint8_t buffer[3];
    st->peek(buffer, sizeof(buffer));
    // 判断头
    if (buffer[0] != 'I' || buffer[1] != 'D' || buffer[2] != '3')
    {
        return false;
    }

    return true;
}

void mp3id3_skip(ColorAudio::Stream *st)
{
    if (mp3id3_have(st))
    {
        uint8_t buffer[16];

        // 读取头
        st->read(buffer, 10);
        uint32_t len = (buffer[6] << 21) + (buffer[7] << 14) + (buffer[8] << 7) + buffer[9];
        st->seek(len + 10 - st->get_pos(), SEEK_CUR);
    }
}

mp3_id3::mp3_id3(ColorAudio::Stream *st)
    : st(st), image(nullptr)
{
    
}

mp3_id3::~mp3_id3()
{
    if (image != nullptr)
    {
        delete image;
        image = nullptr;
    }
}

bool mp3_id3::get_info()
{
    if (!mp3id3_have(st))
    {
        return false;
    }
    uint8_t buffer[16];

    // 读取头
    st->read(buffer, 10);
    version = buffer[3];
    r_version = buffer[4];
    flag = buffer[5];
    length = (buffer[6] << 21) + (buffer[7] << 14) + (buffer[8] << 7) + buffer[9];
    uint32_t len = length;
    uint32_t pos = 0;

    do
    {
        st->read(buffer, 4);
        if (buffer[0] == 0)
        {
            st->seek(len + 10 - st->get_pos(), SEEK_CUR);
            return true;
        }
        pos += 4;
        id3_type type = test_id3_type(reinterpret_cast<char *>(buffer));
        st->read(buffer, 6);
        pos += 6;
        char *output;
        uint32_t size = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
        switch (type)
        {
        case ID3_TITLE_TAG:
        {
            cov_str(st, &output, size);
            if (output != nullptr)
            {
                title = output;
                free(output);
            }
            break;
        }
        case ID3_AUTHER_TAG:
        {
            cov_str(st, &output, size);
            if (output != nullptr)
            {
                auther = output;
                free(output);
            }
            break;
        }
        case ID3_ALBUM_TAG:
        {
            cov_str(st, &output, size);
            if (output != nullptr)
            {
                album = output;
                free(output);
            }
            break;
        }
        case ID3_COMMENT_TAG:
        {
            cov_str(st, &output, size);
            if (output != nullptr)
            {
                comment = output;
                free(output);
            }
            break;
        }
        case ID3_PICTURE_TAG:
        {
            uint8_t encoding = st->read_byte();
            uint32_t mimeType = skip_data(st);
            uint32_t description = skip_data(st);
            uint32_t imageSize = size - (1 + mimeType + 1 + 1 + description + 1);
            image = new data_item(size);
            st->read(image->data, size);
            break;
        }
        default:
        {
            st->seek(size, SEEK_CUR);
            break;
        }
        }
        pos += size;
    } while (pos < len);

    return true;
}