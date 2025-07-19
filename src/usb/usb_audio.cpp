#include "usb_audio.h"
#include "usb_monitor.h"

#include "../player/sound.h"

#include "../lvgl/src/misc/lv_log.h"

#include <errno.h>
#include <stdio.h>

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>

#define UAC2_DEVICE "hw:UAC2Gadget"
#define CHANNELS 2
#define BUFFER_SIZE 1024 * 1024

static std::atomic<bool> running(false);
static snd_pcm_t *capture_handle;
static std::thread *monitor_thread;

static pthread_mutex_t usb_mutex;

static void usb_audio_run()
{
    pthread_mutex_lock(&usb_mutex);

    int err;
    snd_pcm_info_t *info;
    snd_pcm_info_alloca(&info);

    if ((err = snd_pcm_open(&capture_handle, UAC2_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        LV_LOG_ERROR("Cannot open audio device %s (%s)", UAC2_DEVICE, snd_strerror(err));
        return;
    }

    if ((err = snd_pcm_info(capture_handle, info)) < 0)
    {
        LV_LOG_ERROR("info error: %s", snd_strerror(err));
        return;
    }

    // 获取硬件参数
    unsigned int rate = 48000;
    unsigned int channel = 2;
    snd_pcm_format_t format;
    snd_pcm_uframes_t samples = 4096;

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
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
    alsa_set(format, channel, rate);

    LV_LOG_USER("Starting data read thread");

    char *buffer = (char *)malloc(BUFFER_SIZE); // 2 bytes per sample

    unsigned int last_rate = rate;
    snd_pcm_format_t last_format = SND_PCM_FORMAT_S16_LE;
    while (running)
    {
        if ((err = snd_pcm_readi(capture_handle, buffer, samples)) < 0)
        {
            LV_LOG_ERROR("Read error: %s", snd_strerror(err));
            break;
        }

        if (alsa_write_buffer(buffer, err) < 0)
        {
            LV_LOG_ERROR("Write error");
            break;
        }
    }
    free(buffer);
    snd_pcm_close(capture_handle);
    capture_handle = NULL;

    pthread_mutex_unlock(&usb_mutex);
}

void usb_audio(bool enable)
{
    LV_LOG_USER("stop usb Gadget");
    std::system("usbdevice stop");

    std::system("rm /sys/kernel/config/usb_gadget/rockchip/configs/b.1/uac2.usb0");

    std::system("usbdevice stop");

    if (enable)
    {
        LV_LOG_USER("enable uac2");
        std::system("mkdir /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0");

        std::system("echo 0 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/p_chmask");
        std::system("echo 44100,48000,96000,192000 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_srate");
        std::system("echo 2 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_ssize");
        std::system("echo 3 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_chmask");
        std::system("echo -32512 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_volume_min");
        std::system("echo 128 > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/c_volume_res");
        std::system("echo ColorAudio > /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0/function_name");

        std::system("ln -s /sys/kernel/config/usb_gadget/rockchip/functions/uac2.usb0 /sys/kernel/config/usb_gadget/rockchip/configs/b.1/");
    }

    LV_LOG_USER("start usb Gadget");
    std::system("usbdevice start");
}

void usb_audio_stop()
{
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

void usb_audio_test()
{
    pthread_mutex_init(&usb_mutex, NULL);
    LV_LOG_USER("Starting UAC2 Gadget test...");
    usb_audio(true);
    usb_monitor_start();

    for(;;)
    {
        sleep(1);
    }
}
