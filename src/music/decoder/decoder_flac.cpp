#include "decoder_flac.h"
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
static flac_data_t *flac_now;

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

static FLAC__StreamDecoderReadStatus read_cb(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    stream_t *st = flac->st;
    if (stream_can_read(st) == false)
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

static FLAC__StreamDecoderSeekStatus seek_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    stream_t *st = flac->st;
    stream_seek(st, (off_t)absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus tell_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    stream_t *st = flac->st;
    *absolute_byte_offset = (FLAC__uint64)stream_get_pos(st);

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus length_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    stream_t *st = flac->st;
    *stream_length = (FLAC__uint64)stream_get_all_size(st);
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool eof_cb(const FLAC__StreamDecoder *decoder, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    stream_t *st = flac->st;
    return stream_can_read(st) == false;
}

static FLAC__StreamDecoderWriteStatus write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    if (target_time > 0)
    {
        target_time -= (float)frame->header.blocksize / frame->header.sample_rate;
        if (target_time < 0.05)
        {
            play_jump_end();
        }
        else
        {
            return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
        }
    }

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

static void metadata_cb(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    flac_data_t *flac = (flac_data_t *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        uint32_t bps = metadata->data.stream_info.bits_per_sample;
        uint32_t rate = metadata->data.stream_info.sample_rate;
        uint32_t channel = metadata->data.stream_info.channels;

        flac->sample_rate = rate;
        flac->bits_per_sample = bps;

        if (!flac->metadata_only)
        {
            alsa_set(get_format(bps), channel, rate);
        }
        else
        {
            flac->metadata.time = (float)metadata->data.stream_info.total_samples / metadata->data.stream_info.sample_rate;
        }
    }
    else if (flac->metadata_only)
    {
        if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        {
            for (size_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
            {
                uint8_t temp[256];

                FLAC__StreamMetadata_VorbisComment_Entry *enety = &metadata->data.vorbis_comment.comments[i];

                uint32_t len = enety->length;

                char *save = enety->entry;

                size_t eq_pos = strcspn(save, "=");
                if (eq_pos < len)
                {
                    strncpy(temp, enety->entry, eq_pos);
                    temp[eq_pos] = '\0';
                }

                save = enety->entry + eq_pos + 1;
                len -= eq_pos + 1;
                if (len <= 0)
                {
                    continue;
                }
                if (strcasecmp(temp, "title") == 0)
                {
                    uint32_t size = len + 1;
                    flac->metadata.title.data = malloc(size);
                    flac->metadata.title.size = size;
                    strncpy(flac->metadata.title.data, save, len);
                    flac->metadata.title.data[len] = '\0';
                }
                else if (strcasecmp(temp, "artist") == 0)
                {
                    uint32_t size = len + 1;
                    flac->metadata.auther.data = malloc(size);
                    flac->metadata.auther.size = size;
                    strncpy(flac->metadata.auther.data, save, size);
                    flac->metadata.auther.data[len] = '\0';
                }
                else if (strcasecmp(temp, "album") == 0)
                {
                    uint32_t size = len + 1;
                    flac->metadata.album.data = malloc(size);
                    flac->metadata.album.size = size;
                    strncpy(flac->metadata.album.data, save, size);
                    flac->metadata.album.data[len] = '\0';
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
}

static void error_cb(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    LV_LOG_ERROR("Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

void flac_data_close(flac_data_t *flac)
{
    if (!flac)
    {
        return;
    }

    if (flac->metadata_only)
    {
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
    }

    free(flac);
}

flac_data_t *flac_read_metadata(stream_t *st)
{
    flac_data_t *flac_file = calloc(1, sizeof(flac_data_t));
    flac_file->st = st;
    flac_file->metadata_only = true;

    FLAC__StreamDecoder *dec = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(dec, true);
    FLAC__stream_decoder_set_metadata_respond_all(dec);

    FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_stream(dec,
                                                                                 read_cb,
                                                                                 seek_cb,
                                                                                 tell_cb,
                                                                                 length_cb,
                                                                                 eof_cb,
                                                                                 write_cb,
                                                                                 metadata_cb,
                                                                                 error_cb,
                                                                                 flac_file);

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        flac_data_close(flac_file);
        return NULL;
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(dec))
    {
        flac_data_close(flac_file);
        return NULL;
    }

    FLAC__stream_decoder_finish(dec);
    FLAC__stream_decoder_delete(dec);

    return flac_file;
}

bool flac_decode_init(stream_t *st)
{
    flac_now = calloc(1, sizeof(flac_data_t));
    flac_now->st = st;

    if (decoder)
    {
        flac_decode_close();
    }
    decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(decoder, true);
    FLAC__stream_decoder_set_metadata_respond_all(decoder);

    FLAC__StreamDecoderInitStatus init_status = FLAC__stream_decoder_init_stream(decoder,
                                                                                 read_cb,
                                                                                 seek_cb,
                                                                                 tell_cb,
                                                                                 length_cb,
                                                                                 eof_cb,
                                                                                 write_cb,
                                                                                 metadata_cb,
                                                                                 error_cb,
                                                                                 flac_now);

    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        return false;
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
    {
        LV_LOG_ERROR("Decoding interrupted\n");
        return false;
    }

    return true;
}

bool flac_decode_start(stream_t *st)
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

        uint64_t pos;

        if (target_time > 0)
        {
            uint64_t target_sample = (uint64_t)(target_time * flac_now->sample_rate);

            FLAC__stream_decoder_seek_absolute(decoder, target_sample);

            target_time = 0;
            play_jump_end();
        }

        if (!FLAC__stream_decoder_process_single(decoder))
        {
            LV_LOG_ERROR("Decoding interrupted\n");
            return false;
        }

        FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);
        if (state != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC &&
            state != FLAC__STREAM_DECODER_READ_FRAME)
        {
            return true;
        }
    } while (1);

    return true;
}

bool flac_decode_close()
{
    if (flac_now)
    {
        flac_data_close(flac_now);
        flac_now = NULL;
    }
    if (decoder)
    {
        FLAC__stream_decoder_finish(decoder);
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }

    return true;
}