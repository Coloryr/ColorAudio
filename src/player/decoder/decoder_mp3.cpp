#include "decoder_mp3.h"

#include "../player.h"
#include "../sound.h"
#include "../sound_fft.h"
#include "../lvgl/src/misc/lv_log.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <mad.h>

#define BUFSIZE 8192
#define SHIFT 24

using namespace ColorAudio;

static inline int32_t sample_scale(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - SHIFT));

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - SHIFT);
}

static uint32_t get_channels(enum mad_mode mode)
{
    if (mode == MAD_MODE_SINGLE_CHANNEL)
    {
        return 1;
    }

    return 2;
}

static enum mad_flow play_mp3_header(void *data, struct mad_header const *header)
{
    if (target_time > 0)
    {
        if (play_state == MUSIC_STATE_STOP)
        {
            return MAD_FLOW_STOP;
        }
        target_time -= (float)header->duration.fraction / MAD_TIMER_RESOLUTION;
        if (target_time < 0.05)
        {
            play_jump_end();
            return MAD_FLOW_CONTINUE;
        }
        return MAD_FLOW_IGNORE;
    }

    if (play_state == MUSIC_STATE_STOP || play_need_seek)
    {
        return MAD_FLOW_STOP;
    }

    uint32_t samplerate = header->samplerate;

    play_music_bps = header->bitrate;

    alsa_set(SND_PCM_FORMAT_S24, get_channels(header->mode), samplerate);

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow play_mp3_input(void *data, struct mad_stream *st)
{
    if (play_state == MUSIC_STATE_STOP || play_need_seek)
    {
        return MAD_FLOW_STOP;
    }

    if (play_state == MUSIC_STATE_PAUSE)
    {
        return MAD_FLOW_IGNORE;
    }
    
    enum mad_flow ret_code;
    int unproc_data_size;
    int copy_size;

    DecoderMp3 *mp3 = (DecoderMp3 *)data;

    if (mp3->can_read())
    {
        unproc_data_size = st->bufend - st->next_frame;
        mp3->copy(unproc_data_size);
        mad_stream_buffer(st, mp3->get_buffer(), mp3->get_pos());
        ret_code = MAD_FLOW_CONTINUE;
    }
    else
    {
        ret_code = MAD_FLOW_STOP;
    }

    return ret_code;
}

static enum mad_flow play_mp3_output(void *data, const struct mad_header *header, struct mad_pcm *pcm)
{
    time_now += (float)header->duration.fraction / 352800000UL;

    alsa_check_buffer(pcm->length);
    fft_check_buffer(pcm->length);

    uint16_t nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    nchannels = pcm->channels;
    nsamples = pcm->length;

    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];

    uint16_t i = 0;
    uint16_t j = 0;
    while (nsamples--)
    {
        signed int sample;
        // output sample(s) in 24-bit signed little-endian PCM
        sample = sample_scale(*left_ch++);
        sound_buf[i++] = sample;
        sound_fft_buf[j++] = sample;
        if (nchannels == 2)
        {
            sample = sample_scale(*right_ch++);
            sound_buf[i++] = sample;
        }
    }

    fft_fill(0xFFFFFF);

    if (alsa_write() < 0)
    {
        return MAD_FLOW_BREAK;
    }

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow play_mp3_error(void *data, struct mad_stream *mad, struct mad_frame *frame)
{
    time_now += (float)frame->header.duration.fraction / 352800000UL;

    DecoderMp3 *st = (DecoderMp3 *)data;
#ifdef BUILD_ARM
    LV_LOG_ERROR("decoding error 0x%04x (%s) at byte offset %u\n",
                 mad->error, mad_stream_errorstr(mad),
                 mad->this_frame - st->get_buffer());
#else
    LV_LOG_ERROR("decoding error 0x%04x (%s) at byte offset %lu\n",
                 mad->error, mad_stream_errorstr(mad),
                 mad->this_frame - st->get_buffer());
#endif
    return MAD_FLOW_CONTINUE;
}

DecoderMp3::DecoderMp3(ColorAudio::Stream *st) : Decoder(st)
{
    decoder = static_cast<struct mad_decoder *>(malloc(sizeof(struct mad_decoder)));
    buffer = static_cast<uint8_t *>(malloc(STREAM_BUFFER_SIZE));

    mad_decoder_init(decoder, this,
                     play_mp3_input,
                     play_mp3_header,
                     NULL,
                     play_mp3_output,
                     play_mp3_error,
                     NULL);
}

DecoderMp3::~DecoderMp3()
{
    if (decoder)
    {
        mad_decoder_finish(decoder);
        free(decoder);
        decoder = NULL;
    }
    if (buffer)
    {
        free(buffer);
    }
}

bool DecoderMp3::decode_start()
{
    return mad_decoder_run(decoder, MAD_DECODER_MODE_SYNC) == 0;
}

void DecoderMp3::copy(uint32_t data_size)
{
    memcpy(buffer, buffer + pos - data_size, data_size);

    uint32_t copy_size = BUFSIZE - data_size;
    if (st->test_read_size(copy_size) == false)
    {
        copy_size = st->get_less_read();
    }
    st->read(buffer + data_size, copy_size);
    pos = data_size + copy_size;
}