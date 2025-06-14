#include "play_flac.h"
#include "player.h"
#include "sound.h"
#include "utils.h"

#include "lvgl/src/misc/lv_log.h"

#include "FLAC/ordinals.h"
#include "FLAC/stream_encoder.h"
#include "FLAC/metadata.h"
#include "FLAC/format.h"

#include <string.h>
#include <unistd.h>

// 回调函数声明
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    stream *st = (stream *)client_data;
    if (st->pos >= st->size)
    {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    size_t n = stream_read(st, buffer, *bytes);
    if (n == 0)
    {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    else
    {
        *bytes = n;
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
}

FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
    stream *st = (stream *)client_data;
    stream_seek(st, (off_t)absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    stream *st = (stream *)client_data;
    *absolute_byte_offset = (FLAC__uint64)st->pos;

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
    stream *st = (stream *)client_data;

    *stream_length = (FLAC__uint64)st->size;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
    stream *st = (stream *)client_data;
    return st->pos >= st->size ? true : false;
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    uint32_t channels = frame->header.channels;
    uint32_t bps = frame->header.bits_per_sample;

    if (last_pcm_bps != bps || last_pcm_size != frame->header.blocksize)
    {
        if (buf)
        {
            free(buf);
        }
        buf = malloc(sizeof(int32_t) * frame->header.blocksize * 2);
        if (buf1)
        {
            free(buf1);
        }
        buf1 = malloc(sizeof(int32_t) * frame->header.blocksize * 2);
        last_pcm_size = frame->header.blocksize;
        last_pcm_bps = bps;
    }

    if (bps == 16)
    {
        int16_t *buf_1 = (int16_t *)buf;
        uint32_t j = 0;
        uint32_t k = 0;
        for (uint32_t i = 0; i < frame->header.blocksize; i++)
        {
            int16_t sample = buffer[0][i];
            buf_1[k++] = sample;
            buf1[j++] = sample;
            if (channels >= 2)
            {
                sample = buffer[1][i];
                buf_1[k++] = sample;
            }
        }

        fill_fft(frame->header.sample_rate, j, buf1, 0xFFFF);

        if (alsa_write(buf, j) < 0)
        {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }
    else
    {
        uint32_t j = 0;
        uint32_t k = 0;
        for (uint32_t i = 0; i < frame->header.blocksize; i++)
        {
            int32_t sample = buffer[0][i];
            buf[k++] = sample;
            buf1[j++] = sample;
            if (channels >= 2)
            {
                sample = buffer[1][i];
                buf[k++] = sample;
            }
        }

        fill_fft(frame->header.sample_rate, j, buf1, bps == 24 ? 0xFFFFFF : 0xFFFFFFFF);

        if (alsa_write(buf, frame->header.blocksize) < 0)
        {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    time_now += (float)frame->header.blocksize / frame->header.sample_rate;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static snd_pcm_format_t get_format(uint32_t bps)
{
    switch (bps)
    {
    case 16:
        return SND_PCM_FORMAT_S16;
        break;
    case 24:
        return SND_PCM_FORMAT_S24;
    case 32:
        return SND_PCM_FORMAT_S32;
    default:
        return SND_PCM_FORMAT_S16;
        break;
    }
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        uint32_t bps = metadata->data.stream_info.bits_per_sample;
        uint32_t rate = metadata->data.stream_info.sample_rate;
        uint32_t channel = metadata->data.stream_info.channels;

        alsa_set(get_format(bps), channel, rate);

        time_all = (float)metadata->data.stream_info.total_samples / metadata->data.stream_info.sample_rate;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
        for (size_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
        {
            FLAC__StreamMetadata_VorbisComment_Entry enety = metadata->data.vorbis_comment.comments[i];
            char *token = strtok(enety.entry, "="); // 分割符包含空格和等号
            char *current_key = NULL;

            while (token != NULL)
            {
                if (current_key == NULL)
                {
                    current_key = token; // 当前段为键名
                }
                else
                {
                    if (strcasecmp(current_key, "title") == 0)
                    {
                        token = strtok(NULL, "=");
                        play_info_update_title(token, get_length(token) + 1);
                    }
                    else if (strcasecmp(current_key, "artist") == 0)
                    {
                        token = strtok(NULL, "=");
                        play_info_update_auther(token, get_length(token) + 1);
                    }
                    else if (strcasecmp(current_key, "album") == 0)
                    {
                        token = strtok(NULL, "=");
                        play_info_update_album(token, get_length(token) + 1);
                    }
                    break;
                }
            }
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_PICTURE)
    {
        play_info_update_image(metadata->data.picture.data, metadata->data.picture.data_length);
    }
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    LV_LOG_ERROR("Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

FLAC__StreamDecoder *decoder = NULL;
FLAC__StreamDecoderInitStatus init_status = 0;

void flac_decode_init(stream *st)
{
    decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(decoder, true);
    FLAC__stream_decoder_set_metadata_respond_all(decoder);

    init_status = FLAC__stream_decoder_init_stream(decoder, read_callback, seek_callback, tell_callback, length_callback,
                                                   eof_callback, write_callback, metadata_callback, error_callback, st);

    if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
    {
        LV_LOG_ERROR("Decoding interrupted\n");
    }
}

int flac_decode_start(stream *st)
{
    play_set_state(true);
    do
    {
        if (is_pause)
        {
            usleep(10000);
            continue;
        }
        if (is_stop)
        {
            break;
        }
        if (!FLAC__stream_decoder_process_single(decoder))
        {
            LV_LOG_ERROR("Decoding interrupted\n");
        }
    } while (1);

    flac_decode_close();
    play_set_state(false);
}

void flac_decode_close()
{
    if (decoder)
    {
        // 清理资源
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }
}