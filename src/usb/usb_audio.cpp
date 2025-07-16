#include "usb_audio.h"

#include "../player/sound.h"

#include <cstring>
#include <thread>
#include <atomic>
#include <stdio.h>

static std::atomic<bool> usb_audio_running(false);
static std::thread usb_audio_thread;

static int find_usb_audio_device(libusb_device **devs, libusb_device_handle **dev_handle,
                                 int *interface_number, int *alt_setting, unsigned char *endpoint)
{
    libusb_device *dev;
    int i = 0;

    while ((dev = devs[i++]) != nullptr)
    {
        struct libusb_config_descriptor *config;
        int r = libusb_get_active_config_descriptor(dev, &config);
        if (r < 0)
            continue;

        for (int j = 0; j < config->bNumInterfaces; j++)
        {
            const struct libusb_interface *interface = &config->interface[j];
            for (int k = 0; k < interface->num_altsetting; k++)
            {
                const struct libusb_interface_descriptor *altsetting = &interface->altsetting[k];

                // Check for Audio Class interface
                if (altsetting->bInterfaceClass == USB_AUDIO_CLASS &&
                    altsetting->bInterfaceSubClass == USB_AUDIO_SUBCLASS_AUDIOSTREAMING)
                {

                    // Find isochronous endpoint
                    for (int l = 0; l < altsetting->bNumEndpoints; l++)
                    {
                        const struct libusb_endpoint_descriptor *ep = &altsetting->endpoint[l];
                        if ((ep->bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
                        {
                            r = libusb_open(dev, dev_handle);
                            if (r < 0)
                                continue;

                            *interface_number = altsetting->bInterfaceNumber;
                            *alt_setting = altsetting->bAlternateSetting;
                            *endpoint = ep->bEndpointAddress;

                            libusb_free_config_descriptor(config);
                            return 0;
                        }
                    }
                }
            }
        }
        libusb_free_config_descriptor(config);
    }

    return -1;
}

int usb_audio_init(usb_audio_device *dev)
{
    int r = libusb_init(nullptr);
    if (r < 0)
    {
        return r;
    }

    libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    libusb_device **devs;
    ssize_t cnt = libusb_get_device_list(nullptr, &devs);
    if (cnt < 0)
    {
        libusb_exit(nullptr);
        return cnt;
    }

    r = find_usb_audio_device(devs, &dev->dev_handle, &dev->interface_number,
                              &dev->alt_setting, &dev->endpoint);
    libusb_free_device_list(devs, 1);

    if (r < 0)
    {
        libusb_exit(nullptr);
        return r;
    }

    return 0;
}

int usb_audio_set_params(usb_audio_device *dev, snd_pcm_format_t format,
                         unsigned int rate, unsigned char channels)
{
    dev->format = format;
    dev->sample_rate = rate;
    dev->channels = channels;

    // Calculate packet size based on format
    switch (format)
    {
    case SND_PCM_FORMAT_S16_LE:
        dev->packet_size = 2 * channels;
        break;
    case SND_PCM_FORMAT_S24_LE:
        dev->packet_size = 3 * channels;
        break;
    case SND_PCM_FORMAT_S32_LE:
        dev->packet_size = 4 * channels;
        break;
    default:
        return -1;
    }

    return 0;
}

int usb_audio_start(usb_audio_device *dev)
{
    if (usb_audio_running)
    {
        return -1;
    }

    // Claim the interface
    int r = libusb_claim_interface(dev->dev_handle, dev->interface_number);
    if (r < 0)
    {
        return r;
    }

    // Set the alternate setting
    r = libusb_set_interface_alt_setting(dev->dev_handle,
                                         dev->interface_number,
                                         dev->alt_setting);
    if (r < 0)
    {
        libusb_release_interface(dev->dev_handle, dev->interface_number);
        return r;
    }

    // Initialize ALSA with the same parameters
    alsa_init();
    alsa_set(dev->format, dev->channels, dev->sample_rate);

    usb_audio_running = true;
    usb_audio_thread = std::thread([dev]()
                                   {
        unsigned char buffer[1024];
        int transferred;
        
        while (usb_audio_running) {
            int r = libusb_interrupt_transfer(dev->dev_handle, 
                                            dev->endpoint,
                                            buffer, 
                                            sizeof(buffer),
                                            &transferred, 
                                            1000);
            if (r == 0 && transferred > 0) {
                alsa_write_buffer(buffer, transferred / (snd_pcm_format_width(dev->format) / 8 / dev->channels));
            }
        } });

    return 0;
}

int usb_audio_read(usb_audio_device *dev, void *buffer, int length)
{
    int transferred;
    int r = libusb_interrupt_transfer(dev->dev_handle,
                                      dev->endpoint,
                                      (unsigned char *)buffer,
                                      length,
                                      &transferred,
                                      1000);
    return (r == 0) ? transferred : r;
}

void usb_audio_close(usb_audio_device *dev)
{
    usb_audio_running = false;
    if (usb_audio_thread.joinable())
    {
        usb_audio_thread.join();
    }

    if (dev->dev_handle)
    {
        libusb_release_interface(dev->dev_handle, dev->interface_number);
        libusb_close(dev->dev_handle);
        dev->dev_handle = nullptr;
    }

    libusb_exit(nullptr);
}

int usb_test()
{
    usb_audio_device dev = {0};

    if (usb_audio_init(&dev) < 0)
    {
        return -1;
    }

    if (usb_audio_set_params(&dev, SND_PCM_FORMAT_S16_LE, 44100, 2) < 0)
    {
        usb_audio_close(&dev);
        return -1;
    }

    if (usb_audio_start(&dev) < 0)
    {
        usb_audio_close(&dev);
        return -1;
    }

    getchar();

    // usb_audio_close(&dev);
    return 0;
}
