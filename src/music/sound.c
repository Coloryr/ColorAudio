#include "sound.h"
#include "sound_fft.h"

#include "view.h"

#include "lvgl/src/misc/lv_log.h"

// #define ALSA_DEVICE "hw:0,0"
#define ALSA_DEVICE "default"

int32_t *sound_buf;

uint16_t pcm_now_format;
uint32_t pcm_now_size;
uint16_t pcm_now_channels;
uint32_t pcm_now_rate;

static snd_pcm_t *pcm_handle;
static bool pcm_enable;
static bool isset = false;

void alsa_init()
{
    int rc = snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        LV_LOG_USER("ALSA open error: %s\n", snd_strerror(rc));
        return;
    }

    LV_LOG_USER("ALSA open");

    pcm_enable = true;
}

void alsa_check_buffer(uint16_t len)
{
    if (pcm_now_size != len)
    {
        if (sound_buf)
        {
            free(sound_buf);
        }
        sound_buf = malloc(sizeof(int32_t) * len * 2);
        pcm_now_size = len;
    }
}

void alsa_set(snd_pcm_format_t format, uint16_t channels, uint32_t rate)
{
    if (isset)
    {
        return;
    }
#ifdef BUILD_ARM
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 1000000);
#else
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 500000);
#endif

    err = snd_pcm_prepare(pcm_handle);

    pcm_now_channels = channels;
    pcm_now_rate = rate;
    pcm_now_format = snd_pcm_format_width(format);

    LV_LOG_USER("ALSA change, ch:%d, rate:%d, format:%s", channels, rate, snd_pcm_format_name(format));

    view_update_info();

    isset = true;
}

void alsa_reset()
{
    isset = false;
}

int alsa_write()
{
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, sound_buf, pcm_now_size);
    if (frames < 0)
        frames = snd_pcm_recover(pcm_handle, frames, 0);
    if (frames < 0)
        return -1;

    return 0;
}