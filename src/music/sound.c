#include "sound.h"

#include "lvgl/src/misc/lv_log.h"

// #define ALSA_DEVICE "hw:0,0"
#define ALSA_DEVICE "default"

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

void alsa_set(snd_pcm_format_t format, uint16_t channels, uint16_t rate)
{
    if (isset)
    {
        return;
    }
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 50000);
    err = snd_pcm_prepare(pcm_handle);

    LV_LOG_USER("ALSA change, ch:%d, rate:%d, format:%s", channels, rate, snd_pcm_format_name(format));
    isset = true;
}

void alsa_reset()
{
    isset = false;
}

int alsa_write(void *buffer, uint16_t frame_len)
{
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, buffer, frame_len);
    if (frames < 0)
        frames = snd_pcm_recover(pcm_handle, frames, 0);
    if (frames < 0)
        return -1;

    return 0;
}