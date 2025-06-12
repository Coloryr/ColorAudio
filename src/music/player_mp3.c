#include "player_mp3.h"
#include "player.h"
#include "sound.h"
#include "stream.h"
#include "mp3id3.h"

#include "lvgl/src/misc/lv_log.h"

#include <pthread.h>
#include <mad.h>

#define BUFSIZE 8192
#define SHIFT 24

static uint8_t mp3_buffer[8192] = {0};
static uint32_t fbsize = 0;

static struct mad_decoder *decoder;
static pthread_t thread;

static inline signed int scale(mad_fixed_t sample)
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

static enum mad_flow header_func(void *data, struct mad_header const *header)
{
    uint32_t samplerate = header->samplerate;

    alsa_set(SND_PCM_FORMAT_S24, get_channels(header->mode), samplerate);

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow input(void *data, struct mad_stream *st)
{
    int ret_code;
    int unproc_data_size; /*the unprocessed data's size*/
    int copy_size;

    stream *stream1 = (stream *)data;

    if (stream1->pos < stream1->size)
    {
        unproc_data_size = st->bufend - st->next_frame;
        memcpy(mp3_buffer, mp3_buffer + fbsize - unproc_data_size, unproc_data_size);
        copy_size = BUFSIZE - unproc_data_size;
        if (stream1->pos + copy_size > stream1->size)
        {
            copy_size = stream1->size - stream1->pos;
        }
        stream_read(stream1, mp3_buffer + unproc_data_size, copy_size);
        fbsize = unproc_data_size + copy_size;

        /*Hand off the buffer to the mp3 input stream*/
        mad_stream_buffer(st, mp3_buffer, fbsize);
        ret_code = MAD_FLOW_CONTINUE;
    }
    else
    {
        ret_code = MAD_FLOW_STOP;
    }

    return ret_code;
}

static enum mad_flow output(void *data, const struct mad_header *header, struct mad_pcm *pcm)
{
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    nchannels = pcm->channels;
    nsamples = pcm->length;
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];
    uint32_t buf[nsamples * 2];
    int i = 0;
    while (nsamples--)
    {
        signed int sample;
        // output sample(s) in 24-bit signed little-endian PCM
        sample = scale(*left_ch++);
        buf[i++] = sample & 0xFFFFFF;
        if (nchannels == 2)
        {
            sample = scale(*right_ch++);
            buf[i++] = sample & 0xFFFFFF;
        }
    }

    if (alsa_write(buf, pcm->length) < 0)
    {
        return MAD_FLOW_BREAK;
    }

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
    fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %lu\n",
            stream->error, mad_stream_errorstr(stream),
            stream->this_frame - mp3_buffer);

    /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

    return MAD_FLOW_CONTINUE;
}

static void mp3_read_id3(stream *st)
{
    uint8_t buffer[3];
    stream_read(st, buffer, 3);
    if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
    {
        mp3_id3_read(st);
    }
}

void mp3_decode_init(stream *st)
{
    mp3_read_id3(st);
    if (decoder)
    {
        mp3_close();
    }
    decoder = malloc(sizeof(struct mad_decoder));
    mad_decoder_init(decoder, st,
                     input, header_func, NULL /* filter */, output,
                     error, NULL /* message */);
}

int mp3_decode_start(stream *st)
{
    int res = mad_decoder_run(decoder, MAD_DECODER_MODE_SYNC);
    mp3_close();
    return res;
}

void mp3_decode_close()
{
    if (decoder)
    {
        mad_decoder_finish(decoder);
        free(decoder);
        decoder = NULL;
    }
}