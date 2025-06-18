#include "player_flac.h"
#include "player.h"
#include "sound.h"
#include "sound_fft.h"
#include "utils.h"

#include "lvgl/src/misc/lv_log.h"

#include "FLAC/ordinals.h"
#include "FLAC/stream_encoder.h"
#include "FLAC/metadata.h"
#include "FLAC/format.h"

#include <string.h>
#include <unistd.h>

static FLAC__StreamDecoder *decoder = NULL;
static flac_data *flac;

static snd_pcm_format_t _flca_get_format(uint32_t bps)
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

static FLAC__StreamDecoderReadStatus flac_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    stream *st = flac->st;
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

static FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    stream *st = flac->st;
    stream_seek(st, (off_t)absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    stream *st = flac->st;
    *absolute_byte_offset = (FLAC__uint64)st->pos;

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    stream *st = flac->st;
    *stream_length = (FLAC__uint64)st->size;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    stream *st = flac->st;
    return st->pos >= st->size ? true : false;
}

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    uint32_t channels = frame->header.channels;
    uint32_t bps = frame->header.bits_per_sample;
    uint32_t len = frame->header.blocksize;

    alsa_check_buffer(len);
    fft_check_buffer(len);

    if (bps == 16)
    {
        int16_t *buf_1 = (int16_t *)sound_buf;
        uint32_t j = 0;
        uint32_t k = 0;
        for (uint32_t i = 0; i < frame->header.blocksize; i++)
        {
            int16_t sample = buffer[0][i];
            buf_1[k++] = sample;
            sound_fft_buf[j++] = sample;
            if (channels >= 2)
            {
                sample = buffer[1][i];
                buf_1[k++] = sample;
            }
        }

        fft_fill(0xFFFF);

        if (alsa_write() < 0)
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
            sound_buf[k++] = sample;
            sound_fft_buf[j++] = sample;
            if (channels >= 2)
            {
                sample = buffer[1][i];
                sound_buf[k++] = sample;
            }
        }

        fft_fill(bps == 24 ? 0xFFFFFF : 0xFFFFFFFF);

        if (alsa_write() < 0)
        {
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    time_now += (float)frame->header.blocksize / frame->header.sample_rate;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    flac_data *flac = (flac_data *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        uint32_t bps = metadata->data.stream_info.bits_per_sample;
        uint32_t rate = metadata->data.stream_info.sample_rate;
        uint32_t channel = metadata->data.stream_info.channels;

        if (!flac->only_read)
        {
            alsa_set(_flca_get_format(bps), channel, rate);
        }
        flac->metadata.time = (float)metadata->data.stream_info.total_samples / metadata->data.stream_info.sample_rate;
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
                        uint32_t size = get_length(token) + 1;
                        flac->metadata.title.data = malloc(size);
                        flac->metadata.title.size = size;
                        memcpy(flac->metadata.title.data, token, size);
                    }
                    else if (strcasecmp(current_key, "artist") == 0)
                    {
                        token = strtok(NULL, "=");
                        uint32_t size = get_length(token) + 1;
                        flac->metadata.auther.data = malloc(size);
                        flac->metadata.auther.size = size;
                        memcpy(flac->metadata.auther.data, token, size);
                    }
                    else if (strcasecmp(current_key, "album") == 0)
                    {
                        token = strtok(NULL, "=");
                        uint32_t size = get_length(token) + 1;
                        flac->metadata.album.data = malloc(size);
                        flac->metadata.album.size = size;
                        memcpy(flac->metadata.album.data, token, size);
                    }
                    break;
                }
            }
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_PICTURE)
    {
        uint32_t size = metadata->data.picture.data_length;
        flac->metadata.image.data = malloc(size);
        flac->metadata.image.size = size;
        memcpy(flac->metadata.image.data, metadata->data.picture.data, size);
    }
}

static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    LV_LOG_ERROR("Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

void flac_close_data(flac_data *flac)
{
    if (!flac)
    {
        return;
    }

    if (flac->metadata.title.data)
    {
        free(flac->metadata.title.data);
    }
    if (flac->metadata.auther.data)
    {
        free(flac->metadata.auther.data);
    }
    if (flac->metadata.album.data)
    {
        free(flac->metadata.album.data);
    }
    if (flac->metadata.image.data)
    {
        free(flac->metadata.image.data);
    }

    free(flac);
}

flac_data *flac_read_metadata(stream *st)
{
    flac_data *flac_file = malloc(sizeof(flac_data));
    flac_file->st = st;
    flac_file->only_read = true;

    FLAC__StreamDecoder *dec = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(dec, true);
    FLAC__stream_decoder_set_metadata_respond_all(dec);

    FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_stream(dec, flac_read_callback, seek_callback, tell_callback, length_callback,
                                                                                 eof_callback, write_callback, metadata_callback, error_callback, flac_file);

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        flac_close_data(flac_file);
        return NULL;
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(dec))
    {
        flac_close_data(flac_file);
        return NULL;
    }

    FLAC__stream_decoder_finish(dec);
    FLAC__stream_decoder_delete(dec);

    return flac_file;
}

bool flac_decode_init(stream *st)
{
    flac = malloc(sizeof(flac_data));
    flac->st = st;

    if (decoder)
    {
        flac_decode_close();
    }
    decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(decoder, true);
    FLAC__stream_decoder_set_metadata_respond_all(decoder);

    FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_stream(decoder, flac_read_callback, seek_callback, tell_callback, length_callback,
                                                                                 eof_callback, write_callback, metadata_callback, error_callback, flac);

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        return false;
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
    {
        LV_LOG_ERROR("Decoding interrupted\n");
        return false;
    }

    play_info_update_flac(&flac->metadata);

    return true;
}

bool flac_decode_start(stream *st)
{
    do
    {
        if (get_play_state() == MUSIC_STATE_PAUSE)
        {
            usleep(10000);
            continue;
        }
        if (get_play_state() == MUSIC_STATE_STOP)
        {
            break;
        }
        if (!FLAC__stream_decoder_process_single(decoder))
        {
            LV_LOG_ERROR("Decoding interrupted\n");
            return false;
        }
    } while (1);

    return true;
}

bool flac_decode_close()
{
    if (flac)
    {
        flac_close_data(flac);
        flac = NULL;
    }
    if (decoder)
    {
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }

    return true;
}