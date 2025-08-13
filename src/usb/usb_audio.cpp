#include "usb_audio.h"
#include "usb_monitor.h"

#include "../sound/sound.h"
#include "../sound/sound_fft.h"
#include "../ui/usb_view.h"
#include "../ui/info_view.h"
#include "../ui/lang.h"
#include "../config/config.h"

#include "../lvgl/src/misc/lv_log.h"

#include <errno.h>
#include <stdio.h>

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>

#define UAC1_DEVICE "hw:UAC1Gadget"
#define UAC2_DEVICE "hw:UAC2Gadget"
#define CHANNELS 2

using namespace ColorAudio;

static std::atomic<bool> running(false);
static snd_pcm_t *capture_handle;
static std::thread *monitor_thread;

static pthread_mutex_t usb_mutex;

static bool uac2;

static bool open_state;
static uint8_t change_state;

static bool change_uac2;
static const char *change_rate;
static const char *change_bits;

static void usb_audio_run()
{
    pthread_mutex_lock(&usb_mutex);

    LV_LOG_USER("开始读取USB音频");

    int err;
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    unsigned int rate;
    unsigned int channel;
    snd_pcm_format_t format;
    snd_pcm_uframes_t samples;
run:
    if ((err = snd_pcm_open(&capture_handle, uac2 ? UAC2_DEVICE : UAC1_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        LV_LOG_ERROR("Cannot open audio device %s (%s)", uac2 ? UAC2_DEVICE : UAC1_DEVICE, snd_strerror(err));
        pthread_mutex_unlock(&usb_mutex);
        return;
    }

    rate = 48000;
    channel = 2;
    format = SND_PCM_FORMAT_S16_LE;
    samples = 4096;

    snd_pcm_hw_params_any(capture_handle, params);
    snd_pcm_hw_params_set_access(capture_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_rate_near(capture_handle, params, &rate, NULL);
    snd_pcm_hw_params_set_channels_near(capture_handle, params, &channel);
    snd_pcm_hw_params_set_format_first(capture_handle, params, &format);
    snd_pcm_hw_params_set_period_size_near(capture_handle, params, &samples, NULL);

    snd_pcm_hw_params(capture_handle, params);
    snd_pcm_prepare(capture_handle);

    LV_LOG_USER("get rate: %d, samples: %d, format: %s, channel: %d", rate, (int)samples, snd_pcm_format_name(format), channel);

    alsa_reset();
    if (format == SND_PCM_FORMAT_S24_3LE)
    {
        alsa_set(SND_PCM_FORMAT_S24_LE, channel, rate);
    }
    else
    {
        alsa_set(format, channel, rate);
    }

    view_usb_update(true);

    LV_LOG_USER("Starting data read thread");

    uint32_t size = samples * sizeof(int32_t) * 2;
    uint8_t *buffer = static_cast<uint8_t *>(malloc(size));
    uint8_t *output = static_cast<uint8_t *>(malloc(size));

    fft_check_buffer(samples);

    while (running)
    {
        if ((err = snd_pcm_readi(capture_handle, buffer, samples)) < 0)
        {
            LV_LOG_ERROR("Read error: %s", snd_strerror(err));
            break;
        }
        if (format == SND_PCM_FORMAT_S24_3LE)
        {
            for (uint32_t i = 0; i < samples * 2; i++)
            {
                output[i * 4 + 0] = buffer[i * 3 + 0];
                output[i * 4 + 1] = buffer[i * 3 + 1];
                output[i * 4 + 2] = buffer[i * 3 + 2];
                output[i * 4 + 3] = 0x00;
            }
        }
        else
        {
            memcpy(output, buffer, size);
        }

        if (!running)
        {
            break;
        }

        if (format == SND_PCM_FORMAT_S16_LE)
        {
            int16_t *buffer1 = reinterpret_cast<int16_t *>(output);
            for (uint32_t i = 0; i < samples; i++)
            {
                sound_fft_buf[i] = buffer1[i * 2];
            }

            fft_fill_count(0xFFFF, samples);
        }
        else if (format == SND_PCM_FORMAT_S24_3LE)
        {
            // uint32_t index = 0;
            // for (uint32_t i = 0; i < samples * 2; i += 2)
            // {
            //     // Sign extend 24-bit to 32-bit
            //     int32_t sample = (buffer[i * 3 + 2] & 0x80) ? (0xFF << 24) | (buffer[i * 3 + 2] << 16) | (buffer[i * 3 + 1] << 8) | buffer[i * 3 + 0] : (buffer[i * 3 + 2] << 16) | (buffer[i * 3 + 1] << 8) | buffer[i * 3 + 0];
            //     memcpy(&sound_fft_buf[index], &sample, sizeof(sample));
            //     index++;
            //     // sound_fft_buf[i] = buffer1[i * 2];
            // }

            int32_t *buffer1 = reinterpret_cast<int32_t *>(output);
            for (uint32_t i = 0; i < samples; i++)
            {
                int32_t sample = buffer1[i * 2];
                if (sample & 0x800000)
                    sample |= 0xFF000000;
                sound_fft_buf[i] = sample;
            }

            // int32_t *buffer1 = reinterpret_cast<int32_t *>(output);
            // for (uint32_t i = 0; i < samples; i++)
            // {
            //     sound_fft_buf[i] = buffer1[i * 2];
            // }
            fft_fill_count(0xFFFFFF, samples);
        }
        else if (format == SND_PCM_FORMAT_S32_LE)
        {
            int32_t *buffer1 = reinterpret_cast<int32_t *>(output);
            for (uint32_t i = 0; i < samples; i++)
            {
                sound_fft_buf[i] = buffer1[i * 2];
            }

            fft_fill_count(0xFFFFFFFF, samples);
        }

        if (alsa_write_buffer(output, err) < 0)
        {
            LV_LOG_ERROR("Write error");
            break;
        }
    }
    free(buffer);
    free(output);

    snd_pcm_close(capture_handle);
    capture_handle = NULL;

    if (running)
    {
        goto run;
    }

    pthread_mutex_unlock(&usb_mutex);
}

static void usb_audio(bool enable)
{
    LV_LOG_USER("stop usb Gadget");
    std::system("usbdevice stop");

    std::system("rm /sys/kernel/config/usb_gadget/rockchip/configs/b.1/uac1.usb0");
    std::system("rm /sys/kernel/config/usb_gadget/rockchip/configs/b.1/uac2.usb0");

    std::system("usbdevice stop");

    if (enable)
    {
        char temp[256];
        LV_LOG_USER("Starting UAC Gadget");
        if (change_uac2)
        {
            LV_LOG_USER("enable uac2");
            std::system("mkdir /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0");

            std::system("echo 0 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/p_chmask");
            sprintf(temp, "echo %s > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_srate", change_rate);
            std::system(temp);
            sprintf(temp, "echo %s > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_ssize", change_bits);
            std::system(temp);
            std::system("echo 3 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_chmask");
            std::system("echo -32512 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_volume_min");
            std::system("echo 128 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_volume_res");
            std::system("echo ColorAudio > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/function_name");

            std::system("ln -s /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0 /sys/kernel/config/usb_gadget/rockchip/configs/b.1/");
        }
        else
        {
            LV_LOG_USER("enable uac1");
            std::system("mkdir /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0");

            std::system("echo 0 > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/p_chmask");
            sprintf(temp, "echo %s > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/c_srate", change_rate);
            std::system(temp);
            sprintf(temp, "echo %s > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/c_ssize", change_bits);
            std::system(temp);
            std::system("echo 3 > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/c_chmask");
            std::system("echo -32512 > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/c_volume_min");
            std::system("echo 128 > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/c_volume_res");
            std::system("echo ColorAudio > /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0/function_name");

            std::system("ln -s /sys/kernel/config/usb_gadget/rockchip/functions/uac1.usb0 /sys/kernel/config/usb_gadget/rockchip/configs/b.1/");
        }
        uac2 = change_uac2;
    }

    LV_LOG_USER("start usb Gadget");
    std::system("usbdevice start");
}

void usb_audio_stop_run()
{
    view_usb_update(false);

    if (!running)
    {
        return;
    }
    running = false;

    pthread_mutex_lock(&usb_mutex);

    if (monitor_thread)
    {
        delete monitor_thread;
        monitor_thread = NULL;
    }

    pthread_mutex_unlock(&usb_mutex);
}

void usb_audio_start_run()
{
    if (running)
    {
        return;
    }
    running = true;

    pthread_mutex_lock(&usb_mutex);

    if (monitor_thread)
    {
        delete monitor_thread;
        monitor_thread = NULL;
    }

    pthread_mutex_unlock(&usb_mutex);
    monitor_thread = new std::thread(usb_audio_run);
    monitor_thread->detach();
}

void usb_audio_init()
{
    pthread_mutex_init(&usb_mutex, NULL);
}

void usb_audio_start()
{
    usb_monitor_start();
    if (config::get_config_usb_enable())
    {
        change_state = 1;
    }
}

void usb_audio_stop()
{
    usb_monitor_stop();
    running = false;
    open_state = false;
    view_usb_update(false);
}

void usb_audio_exit()
{
    view_top_info_display(now_lang->usb_text9);
    usb_audio(false);
    open_state = false;
    view_top_info_close();
}

void usb_audio_change(bool state)
{
    change_state = state ? 1 : 2;
}

void usb_audio_set_mode(bool uac2)
{
    change_uac2 = uac2;
}

void usb_audio_set_rate(const char *rate)
{
    change_rate = rate;
}

void usb_audio_set_bits(const char *bits)
{
    change_bits = bits;
}

void usb_audio_tick()
{
    if (change_state != 0)
    {
        if (change_state == 1 && open_state != true)
        {
            view_top_info_display(now_lang->usb_text8);
            usb_audio(true);
            open_state = true;
            view_top_info_close();
        }
        else if (change_state == 2 && open_state != false)
        {
            view_top_info_display(now_lang->usb_text9);
            running = false;
            usb_audio(false);
            open_state = false;
            view_top_info_close();
        }

        change_state = 0;
    }
}

bool usb_audio_is_connect()
{
    return running;
}