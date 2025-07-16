#include "sound.h"
#include "sound_fft.h"

#include "../ui/view_state.h"
#include "../config/config.h"

#include "../lvgl/src/misc/lv_log.h"

#include <alsa/asoundlib.h>
#include <math.h>

using namespace ColorAudio;

// #define ALSA_DEVICE "hw:0,0"
#define ALSA_DEVICE "default"

#define TARGET_NAME "Master Playback Volume"

#define TARGET_RANGE_MIN 0

#ifdef BUILD_ARM
#define TARGET_RANGE_MAX 255
#else
#define TARGET_RANGE_MAX 65536
#endif

int32_t *sound_buf;

uint16_t pcm_now_format;
uint32_t pcm_now_size;
uint16_t pcm_now_channels;
uint32_t pcm_now_rate;

static snd_pcm_t *pcm_handle;
static bool pcm_enable;
static bool pcm_ctl;
static bool isset = false;

static snd_ctl_t *ctl_handle;
static snd_ctl_elem_id_t *ctl_id;

#ifdef BUILD_ARM
unsigned int tlv[256];
unsigned int *tlvp;
long db_min = -1;
long db_max = -1;
#endif

static long min_val, max_val;

static bool find_controls()
{
    bool found = false;
    int err;
    snd_ctl_elem_list_t *list;
    snd_ctl_elem_list_alloca(&list);

    snd_ctl_elem_list(ctl_handle, list);
    int count = snd_ctl_elem_list_get_count(list);
    snd_ctl_elem_list_alloc_space(list, count);

    snd_ctl_elem_list(ctl_handle, list);

    for (int i = 0; i < count; i++)
    {
        snd_ctl_elem_id_t *id;
        snd_ctl_elem_id_alloca(&id);
        snd_ctl_elem_list_get_id(list, i, id);

        const char *name = snd_ctl_elem_id_get_name(id);
        snd_ctl_elem_iface_t iface = snd_ctl_elem_id_get_interface(id);

        if (strcmp(name, TARGET_NAME) != 0 || iface != SND_CTL_ELEM_IFACE_MIXER)
        {
            continue;
        }

        snd_ctl_elem_info_t *info;
        snd_ctl_elem_info_alloca(&info);
        snd_ctl_elem_info_set_id(info, id);
        if ((err = snd_ctl_elem_info(ctl_handle, info)) < 0)
        {
            continue;
        }

        if (snd_ctl_elem_info_get_type(info) == SND_CTL_ELEM_TYPE_INTEGER &&
            snd_ctl_elem_info_get_count(info) == 2)
        {
#ifdef BUILD_ARM
            if (!snd_ctl_elem_info_is_tlv_readable(info))
            {
                LV_LOG_ERROR("TLV read not supported");
                continue;
            }
            snd_ctl_elem_id_malloc(&ctl_id);
            snd_ctl_elem_list_get_id(list, i, ctl_id);
            min_val = snd_ctl_elem_info_get_min(info);
            max_val = snd_ctl_elem_info_get_max(info);
            snd_ctl_elem_tlv_read(ctl_handle, ctl_id, tlv, sizeof(tlv));
            snd_tlv_parse_dB_info(tlv, sizeof(tlv), &tlvp);
            snd_tlv_get_dB_range(tlvp, min_val, max_val, &db_min, &db_max);
            LV_LOG_USER("tlv db: %ld-%ld val %ld-%ld", db_min, db_max, min_val, max_val);
            found = true;
            break;
#else
            min_val = snd_ctl_elem_info_get_min(info);
            max_val = snd_ctl_elem_info_get_max(info);

            if (min_val == TARGET_RANGE_MIN && max_val == TARGET_RANGE_MAX)
            {
                found = true;
                snd_ctl_elem_id_malloc(&ctl_id);
                snd_ctl_elem_list_get_id(list, i, ctl_id);
                break;
            }
#endif
        }
    }

    snd_ctl_elem_list_free_space(list);

    return found;
}

void alsa_init()
{
    int err = snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA open error: %s\n", snd_strerror(err));
        return;
    }

    LV_LOG_USER("ALSA open");

    err = snd_ctl_open(&ctl_handle, ALSA_DEVICE, 0);
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA Control open error: %s\n", snd_strerror(err));
        return;
    }
    pcm_ctl = find_controls();

    float volume = config::get_config_volume();
    if (volume >= 0 && volume <= 100)
    {
        alsa_set_volume(volume);
    }

    pcm_enable = true;
}

float alsa_get_volume()
{
    if (!pcm_ctl)
    {
        return 0;
    }
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, ctl_id);
    int err = snd_ctl_elem_read(ctl_handle, control);
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA control read error: %s\n", snd_strerror(err));
        return 0;
    }

    long target_val = snd_ctl_elem_value_get_integer(control, 0);

#ifdef BUILD_ARM
    long db = 0;
    snd_tlv_convert_to_dB(tlvp, min_val, max_val, target_val, &db);
    float value = (100.0 * pow(10, (db - db_max) / 6000.0));
    LV_LOG_USER("now volume: %f", value);
    return value;
#else
    return ((float)target_val - min_val) / (max_val - min_val) * 100;
#endif
}

void alsa_set_volume(float value)
{
    if (!pcm_ctl)
    {
        return;
    }
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, ctl_id);

    long target_val;
#ifdef BUILD_ARM
    float db;
    double tmp = 6000.0 * log10(value / 100.0);
    if (value > 0)
    {
        db = ceil(tmp) + db_max;
    }
    else
    {
        db = floor(tmp) + db_max;
    }
    snd_tlv_convert_from_dB(tlvp, min_val, max_val, db, &target_val, ((db > 0) ? 1 : -1));
#else
    target_val = min_val + (max_val - min_val) * value / 100;
#endif

    snd_ctl_elem_value_set_integer(control, 0, target_val);
    snd_ctl_elem_value_set_integer(control, 1, target_val);

    int err = snd_ctl_elem_write(ctl_handle, control);

    config::set_config_volume(value);
    config::save_config();
}

void alsa_check_buffer(uint16_t len)
{
    if (pcm_now_size != len)
    {
        if (sound_buf)
        {
            free(sound_buf);
        }
        sound_buf = static_cast<int32_t *>(malloc(sizeof(int32_t) * len * 2));
        pcm_now_size = len;
    }
}

void alsa_set(snd_pcm_format_t format, uint16_t channels, uint32_t rate)
{
    if (isset)
    {
        return;
    }

    snd_pcm_reset(pcm_handle);
#ifdef BUILD_ARM
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 1000000);
#else
    snd_pcm_close(pcm_handle);
    snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 500000);
#endif
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }

    err = snd_pcm_prepare(pcm_handle);
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }

    pcm_now_channels = channels;
    pcm_now_rate = rate;
    pcm_now_format = snd_pcm_format_width(format);

    LV_LOG_USER("ALSA change, ch:%d, rate:%d, format:%s", channels, rate, snd_pcm_format_name(format));

    view_update_info();

    isset = true;
}

void alsa_clear()
{
    snd_pcm_reset(pcm_handle);
}

void alsa_ready()
{
    snd_pcm_prepare(pcm_handle);
}

void alsa_reset()
{
    isset = false;

    snd_pcm_reset(pcm_handle);
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

int alsa_write_buffer(const void *buffer, size_t samples)
{
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, buffer, samples);
    if (frames < 0)
        frames = snd_pcm_recover(pcm_handle, frames, 0);
    if (frames < 0)
        return -1;

    return 0;
}