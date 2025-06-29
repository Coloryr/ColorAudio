#include "player_mp3.h"
#include "mp3_header.h"
#include "player.h"
#include "sound.h"
#include "sound_fft.h"
#include "stream.h"
#include "stream_buffer.h"
#include "mp3id3.h"

#include "lvgl/src/misc/lv_log.h"

#include <pthread.h>
#include <mad.h>

#define BUFSIZE 8192
#define SHIFT 24

static struct mad_decoder *decoder = NULL;

static pthread_t thread;

static stream_buffer_t *buffer_stream;

static inline signed int mp3_sample_scale(mad_fixed_t sample)
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

static int get_channels(enum mad_mode mode)
{
    if (mode == MAD_MODE_SINGLE_CHANNEL)
    {
        return 1;
    }

    return 2;
}

static enum mad_flow play_mp3_header(void *data, struct mad_header const *header)
{
    if (get_play_state() == MUSIC_STATE_STOP)
    {
        return MAD_FLOW_STOP;
    }

    uint32_t samplerate = header->samplerate;

    play_mp3_bps = header->bitrate;

    alsa_set(SND_PCM_FORMAT_S24, get_channels(header->mode), samplerate);

    if (target_time > 0)
    {
        target_time -= (float)header->duration.fraction / MAD_TIMER_RESOLUTION;
        if (target_time < 0.05)
        {
            play_jump_end();
            return MAD_FLOW_CONTINUE;
        }
        return MAD_FLOW_IGNORE;
    }

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow play_mp3_input(void *data, struct mad_stream *st)
{
    if (get_play_state() == MUSIC_STATE_PAUSE)
    {
        return MAD_FLOW_IGNORE;
    }

    if (get_play_state() == MUSIC_STATE_STOP)
    {
        return MAD_FLOW_STOP;
    }
    int ret_code;
    int unproc_data_size;
    int copy_size;

    stream_buffer_t *stream1 = (stream_buffer_t *)data;

    if (stream_can_read(stream1->stream))
    {
        unproc_data_size = st->bufend - st->next_frame;
        memcpy(stream1->buffer, stream1->buffer + stream1->buffer_pos - unproc_data_size, unproc_data_size);
        copy_size = BUFSIZE - unproc_data_size;
        if (stream_test_read_size(stream1->stream, copy_size) == false)
        {
            copy_size = stream_get_less_read(stream1->stream);
        }
        stream_read(stream1->stream, stream1->buffer + unproc_data_size, copy_size);
        stream1->buffer_pos = unproc_data_size + copy_size;

        mad_stream_buffer(st, stream1->buffer, stream1->buffer_pos);
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
        sample = mp3_sample_scale(*left_ch++);
        sound_buf[i++] = sample;
        sound_fft_buf[j++] = sample;
        if (nchannels == 2)
        {
            sample = mp3_sample_scale(*right_ch++);
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

    stream_buffer_t *st = (stream_buffer_t *)data;
    LV_LOG_ERROR("decoding error 0x%04x (%s) at byte offset %lu\n",
                 mad->error, mad_stream_errorstr(mad),
                 mad->this_frame - st->buffer);

    return MAD_FLOW_CONTINUE;
}

bool mp3_decode_init(stream_t *st)
{
    if (decoder)
    {
        mp3_decode_close();
    }
    decoder = malloc(sizeof(struct mad_decoder));
    buffer_stream = stream_create_with_buffer(st);
    mad_decoder_init(decoder, buffer_stream,
                     play_mp3_input,
                     play_mp3_header,
                     NULL,
                     play_mp3_output,
                     play_mp3_error, 
                     NULL);

    return true;
}

bool mp3_decode_start(stream_t *st)
{
    return mad_decoder_run(decoder, MAD_DECODER_MODE_SYNC) == 0;
}

bool mp3_decode_close()
{
    if (decoder)
    {
        mad_decoder_finish(decoder);
        free(decoder);
        decoder = NULL;
    }
    if (buffer_stream)
    {
        stream_buffer_close(buffer_stream);
        buffer_stream = NULL;
    }

    return true;
}