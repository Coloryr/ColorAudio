#include "usb_monitor.h"
#include "usb_audio.h"

#include "../player/sound.h"

#include "../lvgl/src/misc/lv_log.h"

#include <libudev.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <thread>

#define UAC1_DEVPATH "/devices/virtual/u_audio/UAC1_Gadget 0"
#define UAC2_DEVPATH "/devices/virtual/u_audio/UAC2_Gadget 0"

static struct udev *udev;
static struct udev_monitor *mon;
static struct pollfd fds[1];

static std::atomic<bool> running(true);

void usb_monitor_run()
{
    while (running)
    {
        int ret = poll(fds, 1, -1);
        if (ret < 0)
        {
            LV_LOG_ERROR("poll 错误");
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev)
            {
                const char *devpath = udev_device_get_devpath(dev);
                const char *action = udev_device_get_action(dev);
                const char *subsystem = udev_device_get_subsystem(dev);

                if (devpath && (strcmp(devpath, UAC1_DEVPATH) == 0 || strcmp(devpath, UAC2_DEVPATH) == 0))
                {
                    const char *state = udev_device_get_property_value(dev, "USB_STATE");
                    if (state != NULL)
                    {
                        if (strcmp(state, "SET_VOLUME") == 0)
                        {
                            const char *volume = udev_device_get_property_value(dev, "VOLUME");
                            if (volume != NULL)
                            {
                                int16_t num;
                                sscanf(volume, "0x%hx", &num);
                                num /= -128;
                                num = 255 - num;
                                alsa_set_volume_db(num);
                            }
                        }
                        else if (strcmp(state, "SET_MUTE") == 0)
                        {
                            const char *mute = udev_device_get_property_value(dev, "MUTE");
                            if (mute != NULL)
                            {
                                if (strcmp(mute, "1") == 0)
                                {
                                    alsa_set_volume(0);
                                }
                            }
                        }
                        else if (strcmp(state, "SET_INTERFACE") == 0)
                        {
                            const char *ststate = udev_device_get_property_value(dev, "STREAM_STATE");
                            if (ststate != NULL)
                            {
                                if (strcmp(ststate, "OFF") == 0)
                                {
                                    usb_audio_stop();
                                }
                                else if (strcmp(ststate, "ON") == 0)
                                {
                                    usb_audio_stop();
                                    usb_audio_start_run();
                                }
                            }
                        }
                    }
                }

                udev_device_unref(dev);
            }
        }
    }
}

void usb_monitor_start()
{
    int ret;

    udev = udev_new();
    if (!udev)
    {
        LV_LOG_ERROR("无法创建 udev 上下文");
        return;
    }

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon)
    {
        LV_LOG_ERROR("无法创建 udev 监控器");
        udev_unref(udev);
        return;
    }

    if (udev_monitor_enable_receiving(mon) < 0)
    {
        LV_LOG_ERROR("无法启用 udev 监控");
        udev_monitor_unref(mon);
        udev_unref(udev);
        return;
    }

    fds[0].fd = udev_monitor_get_fd(mon);
    fds[0].events = POLLIN;

    std::thread monitor_thread(usb_monitor_run);
    monitor_thread.detach();
}

void usb_monitor_stop()
{
    if (mon)
    {
        udev_monitor_unref(mon);
        mon = NULL;
    }
    if (udev)
    {
        udev_unref(udev);
        udev = NULL;
    }
}