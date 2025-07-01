#include "decoder_flac.h"

#include "../player.h"
#include "../sound.h"
#include "../sound_fft.h"
#include "../common/data_item.h"

#include "../lvgl/src/misc/lv_log.h"

#include <string.h>
#include <unistd.h>

#include <string>

using namespace ColorAudio;

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

DecoderFlac::DecoderFlac(ColorAudio::Stream *st) : Decoder(st)
{
    set_md5_checking(true);
}

DecoderFlac::~DecoderFlac()
{
    finish();
}

FLAC__StreamDecoderReadStatus DecoderFlac::read_callback(FLAC__byte buffer[], size_t *bytes)
{
    if (st->can_read() == false)
    {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    size_t n = st->read(buffer, *bytes);
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

FLAC__StreamDecoderSeekStatus DecoderFlac::seek_callback(FLAC__uint64 absolute_byte_offset)
{
    st->seek((off_t)absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus DecoderFlac::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
    *absolute_byte_offset = (FLAC__uint64)st->get_pos();

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus DecoderFlac::length_callback(FLAC__uint64 *stream_length)
{
    *stream_length = (FLAC__uint64)st->get_all_size();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool DecoderFlac::eof_callback()
{
    return st->can_read() == false;
}

FLAC__StreamDecoderWriteStatus DecoderFlac::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])
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

void DecoderFlac::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        uint32_t bps = metadata->data.stream_info.bits_per_sample;
        uint32_t rate = metadata->data.stream_info.sample_rate;
        uint32_t channel = metadata->data.stream_info.channels;

        sample_rate = rate;
        bits_per_sample = bps;

        alsa_set(get_format(bps), channel, rate);
    }
}

void DecoderFlac::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
    LV_LOG_ERROR("Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

bool DecoderFlac::decode_start()
{
    if (init() != FLAC__STREAM_DECODER_INIT_STATUS_OK || !process_until_end_of_metadata())
    {
        LV_LOG_ERROR("Decoding interrupted\n");
        return false;
    }

    do
    {
        if (play_state == MUSIC_STATE_PAUSE)
        {
            usleep(10000);
            continue;
        }
        if (play_state == MUSIC_STATE_STOP)
        {
            break;
        }

        uint64_t pos;

        if (target_time > 0)
        {
            uint64_t target_sample = (uint64_t)(target_time * sample_rate);

            seek_absolute(target_sample);

            target_time = 0;
            play_jump_end();
        }

        if (!process_single())
        {
            LV_LOG_ERROR("Decoding interrupted\n");
            return false;
        }

        FLAC__StreamDecoderState state = get_state();
        if (state != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC &&
            state != FLAC__STREAM_DECODER_READ_FRAME)
        {
            return true;
        }
    } while (1);

    return true;
}
