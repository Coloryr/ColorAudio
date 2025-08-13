#include "sound.h"
#include "sound_fft.h"

#include "../config/config.h"
#include "../ui/music_view.h"
#include "../io/gpio.h"
#include "../io/event.h"
#include "../lvgl/src/misc/lv_log.h"

#include <alsa/asoundlib.h>
#include <math.h>

using namespace ColorAudio;

#ifdef BUILD_ARM
#define ALSA_DEVICE_A "hw:0"
#define ALSA_DEVICE_B "hw:1"
#define ALSA_DEVICE_CTL_A "hw:0"
#define ALSA_DEVICE_CTL_B "hw:1"
#else
#define ALSA_DEVICE "default"
#endif

#define TARGET_NAME "Master Playback Volume"
#define JACK_TARGET_NAME "Headphone Jack"

#define TARGET_RANGE_MIN 0

#ifdef BUILD_ARM
#define TARGET_RANGE_MAX 255
#else
#define TARGET_RANGE_MAX 65536
#endif

int32_t *sound_buf;

uint16_t pcm_now_format_size;
uint32_t pcm_now_size;
uint16_t pcm_now_channels;
uint32_t pcm_now_rate;
snd_pcm_format_t pcm_now_format;

#ifdef BUILD_ARM
static snd_pcm_t *pcm_handle_a;
static snd_pcm_t *pcm_handle_b;

static snd_ctl_t *ctl_handle_a;
static snd_ctl_elem_id_t *ctl_id_a;
static snd_ctl_t *ctl_handle_b;
static snd_ctl_elem_id_t *ctl_id_b;
#else
static snd_pcm_t *pcm_handle;
static snd_ctl_t *ctl_handle;
static snd_ctl_elem_id_t *ctl_id;
#endif
static bool pcm_enable;
static bool pcm_ctl;
static bool isset = false;

#ifdef BUILD_ARM
unsigned int tlv[256];
unsigned int *tlvp;
long db_min = -1;
long db_max = -1;
bool enable_double;

uint32_t pcm_now_db;
#endif

static long min_val, max_val;

static bool find_controls(snd_ctl_t *ctl, snd_ctl_elem_id_t *ctl_id, bool *on)
{
    bool found = false;
    int err;
    snd_ctl_elem_list_t *list;
    snd_ctl_elem_list_alloca(&list);

    snd_ctl_elem_list(ctl, list);
    int count = snd_ctl_elem_list_get_count(list);
    snd_ctl_elem_list_alloc_space(list, count);

    snd_ctl_elem_list(ctl, list);

    for (int i = 0; i < count; i++)
    {
        snd_ctl_elem_id_t *id;
        snd_ctl_elem_id_alloca(&id);
        snd_ctl_elem_list_get_id(list, i, id);

        snd_ctl_elem_info_t *info;
        snd_ctl_elem_info_alloca(&info);
        snd_ctl_elem_info_set_id(info, id);

        if ((err = snd_ctl_elem_info(ctl, info)) < 0)
        {
            continue;
        }

        const char *name = snd_ctl_elem_id_get_name(id);
        snd_ctl_elem_iface_t iface = snd_ctl_elem_id_get_interface(id);

        if (strcmp(name, JACK_TARGET_NAME) == 0 && iface == SND_CTL_ELEM_IFACE_CARD)
        {
#ifdef BUILD_ARM
            if (snd_ctl_elem_info_get_type(info) == SND_CTL_ELEM_TYPE_BOOLEAN)
            {
                snd_ctl_elem_value_t *value;
                snd_ctl_elem_value_alloca(&value);
                snd_ctl_elem_value_set_id(value, id);
                snd_ctl_elem_read(ctl, value);

                *on = snd_ctl_elem_value_get_boolean(value, 0);
            }
#endif
        }
        else if (strcmp(name, TARGET_NAME) == 0 && iface == SND_CTL_ELEM_IFACE_MIXER)
        {
            if (snd_ctl_elem_info_get_type(info) == SND_CTL_ELEM_TYPE_INTEGER &&
                snd_ctl_elem_info_get_count(info) == 2)
            {
#ifdef BUILD_ARM
                if (!snd_ctl_elem_info_is_tlv_readable(info))
                {
                    LV_LOG_ERROR("TLV read not supported");
                    continue;
                }
                snd_ctl_elem_list_get_id(list, i, ctl_id);
                min_val = snd_ctl_elem_info_get_min(info);
                max_val = snd_ctl_elem_info_get_max(info);
                snd_ctl_elem_tlv_read(ctl, ctl_id, tlv, sizeof(tlv));
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
                    snd_ctl_elem_list_get_id(list, i, ctl_id);
                    break;
                }
#endif
            }
        }
    }

    snd_ctl_elem_list_free_space(list);

    return found;
}

void alsa_init()
{
#ifdef BUILD_ARM
    int err = snd_pcm_open(&pcm_handle_a, ALSA_DEVICE_A, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA open error: %s\n", snd_strerror(err));
        return;
    }
    err = snd_pcm_open(&pcm_handle_b, ALSA_DEVICE_B, SND_PCM_STREAM_PLAYBACK, 0);
#else
    int err = snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
#endif
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA open error: %s\n", snd_strerror(err));
        return;
    }

    LV_LOG_USER("ALSA open");

#ifdef BUILD_ARM
    err = snd_ctl_open(&ctl_handle_a, ALSA_DEVICE_A, 0);
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA Control open error: %s\n", snd_strerror(err));
        return;
    }
    err = snd_ctl_open(&ctl_handle_b, ALSA_DEVICE_B, 0);
#else
    err = snd_ctl_open(&ctl_handle, ALSA_DEVICE, 0);
#endif
    if (err < 0)
    {
        LV_LOG_ERROR("ALSA Control open error: %s\n", snd_strerror(err));
        return;
    }
#ifdef BUILD_ARM
    snd_ctl_elem_id_malloc(&ctl_id_a);
    snd_ctl_elem_id_malloc(&ctl_id_b);
    pcm_ctl = find_controls(ctl_handle_a, ctl_id_a, &headphone_1_in) &&
              find_controls(ctl_handle_b, ctl_id_b, &headphone_2_in);
    enable_double = config::get_config_codec_double();
#else
    snd_ctl_elem_id_malloc(&ctl_id);
    pcm_ctl = find_controls(ctl_handle, ctl_id, NULL);
#endif
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
#ifdef BUILD_ARM
    snd_ctl_elem_value_set_id(control, ctl_id_a);
    int err = snd_ctl_elem_read(ctl_handle_a, control);
#else
    snd_ctl_elem_value_set_id(control, ctl_id);
    int err = snd_ctl_elem_read(ctl_handle, control);
#endif
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

    long target_val;
#ifdef BUILD_ARM
    snd_ctl_elem_value_set_id(control, ctl_id_a);
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
    snd_ctl_elem_value_set_id(control, ctl_id);
    target_val = min_val + (max_val - min_val) * value / 100;
#endif

    snd_ctl_elem_value_set_integer(control, 0, target_val);
    snd_ctl_elem_value_set_integer(control, 1, target_val);
#ifdef BUILD_ARM
    snd_ctl_elem_write(ctl_handle_a, control);
    if (enable_double)
    {
        snd_ctl_elem_value_set_id(control, ctl_id_b);
        snd_ctl_elem_write(ctl_handle_b, control);
    }
#else
    snd_ctl_elem_write(ctl_handle, control);
#endif

    config::set_config_volume(value);
    config::save_config();
}

void alsa_set_volume_db(long value)
{
    if (!pcm_ctl)
    {
        return;
    }

    snd_ctl_elem_value_t *control;
    snd_ctl_elem_value_alloca(&control);
#ifdef BUILD_ARM
    pcm_now_db = value;
    snd_ctl_elem_value_set_id(control, ctl_id_a);
#else
    snd_ctl_elem_value_set_id(control, ctl_id);
#endif
    snd_ctl_elem_value_set_integer(control, 0, value);
    snd_ctl_elem_value_set_integer(control, 1, value);
#ifdef BUILD_ARM
    snd_ctl_elem_write(ctl_handle_a, control);
#else
    snd_ctl_elem_write(ctl_handle, control);
#endif
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

#ifdef BUILD_ARM
    snd_pcm_drain(pcm_handle_a);
    set_amp_power(false);
    snd_pcm_drop(pcm_handle_a);
    snd_pcm_reset(pcm_handle_a);
    int err = snd_pcm_set_params(pcm_handle_a, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 0, 100000);
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }
    if (enable_double)
    {
        snd_pcm_drop(pcm_handle_b);
        snd_pcm_reset(pcm_handle_b);
        err = snd_pcm_set_params(pcm_handle_b, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 0, 100000);
        if (err != 0)
        {
            LV_LOG_ERROR("ALSA format set fail");
        }
    }
    set_amp_power(true);
#else
    snd_pcm_reset(pcm_handle);
    snd_pcm_close(pcm_handle);
    snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    int err = snd_pcm_set_params(pcm_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 1, 500000);
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }
#endif

#ifdef BUILD_ARM
    err = snd_pcm_prepare(pcm_handle_a);
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }
    if (enable_double)
    {
        err = snd_pcm_prepare(pcm_handle_b);
        if (err != 0)
        {
            LV_LOG_ERROR("ALSA format set fail");
        }
    }
#else
    err = snd_pcm_prepare(pcm_handle);
    if (err != 0)
    {
        LV_LOG_ERROR("ALSA format set fail");
    }
#endif

    pcm_now_channels = channels;
    pcm_now_rate = rate;
    pcm_now_format = format;
    pcm_now_format_size = snd_pcm_format_width(format);

    LV_LOG_USER("ALSA change, ch:%d, rate:%d, format:%s", channels, rate, snd_pcm_format_name(format));

    view_music_update_info();

    isset = true;
}

void alsa_clear()
{
#ifdef BUILD_ARM
    snd_pcm_reset(pcm_handle_a);
    if (enable_double)
    {
        snd_pcm_reset(pcm_handle_b);
    }
#else
    snd_pcm_reset(pcm_handle);
#endif
}

void alsa_ready()
{
#ifdef BUILD_ARM
    snd_pcm_prepare(pcm_handle_a);
    if (enable_double)
    {
        snd_pcm_prepare(pcm_handle_b);
    }
#else
    snd_pcm_prepare(pcm_handle);
#endif
}

void alsa_reset()
{
    isset = false;
}

int alsa_write()
{
#ifdef BUILD_ARM
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle_a, sound_buf, pcm_now_size);
#else
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, sound_buf, pcm_now_size);
#endif
    if (frames < 0)
#ifdef BUILD_ARM
        frames = snd_pcm_recover(pcm_handle_a, frames, 0);
#else
        frames = snd_pcm_recover(pcm_handle, frames, 0);
#endif
    if (frames < 0)
        return -1;

    return 0;
}

int alsa_write_buffer(const void *buffer, size_t samples)
{
#ifdef BUILD_ARM
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle_a, buffer, samples);
#else
    snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, buffer, samples);
#endif
    if (frames < 0)
#ifdef BUILD_ARM
        frames = snd_pcm_recover(pcm_handle_a, frames, 0);
#else
        frames = snd_pcm_recover(pcm_handle, frames, 0);
#endif
    if (frames < 0)
        return -1;

    return 0;
}

#ifdef BUILD_ARM
void alsa_codec_double_change()
{
    set_amp_power(false);
    if (config::get_config_codec_double())
    {
        enable_double = true;
        if (isset)
        {
            snd_pcm_set_params(pcm_handle_b, pcm_now_format, SND_PCM_ACCESS_RW_INTERLEAVED, pcm_now_channels, pcm_now_rate, 0, 100000);
            snd_pcm_prepare(pcm_handle_b);
        }
    }
    else
    {
        enable_double = false;
        if (isset)
        {
            snd_pcm_drop(pcm_handle_b);
            snd_pcm_reset(pcm_handle_b);

            // snd_pcm_close(pcm_handle_b);
            // snd_pcm_open(&pcm_handle_b, ALSA_DEVICE_B, SND_PCM_STREAM_PLAYBACK, 0);
        }
    }
    set_amp_power(true);
}
#endif