#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__

#include <alsa/asoundlib.h>
#include <libusb-1.0/libusb.h>

// USB Audio Class specifications
#define USB_AUDIO_CLASS 1
#define USB_AUDIO_SUBCLASS_AUDIOCONTROL 1
#define USB_AUDIO_SUBCLASS_AUDIOSTREAMING 2
#define USB_AUDIO_PROTOCOL_UNDEFINED 0

// Audio Format Types
#define USB_AUDIO_FORMAT_TYPE_I 0x01

// Audio Data Formats
#define USB_AUDIO_FORMAT_PCM 0x01

typedef struct {
    libusb_device_handle* dev_handle;
    int interface_number;
    int alt_setting;
    unsigned char endpoint;
    snd_pcm_format_t format;
    unsigned int sample_rate;
    unsigned char channels;
    unsigned int packet_size;
} usb_audio_device;

int usb_audio_init(usb_audio_device* dev);
int usb_audio_set_params(usb_audio_device* dev, snd_pcm_format_t format, 
                        unsigned int rate, unsigned char channels);
int usb_audio_start(usb_audio_device* dev);
int usb_audio_read(usb_audio_device* dev, void* buffer, int length);
void usb_audio_close(usb_audio_device* dev);

int usb_test();

#endif // __USB_AUDIO_H__
