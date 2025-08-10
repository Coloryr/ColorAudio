#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__

void usb_audio_start_run();
void usb_audio_stop_run();

void usb_audio_init();
void usb_audio_start();
void usb_audio_stop();
void usb_audio_tick();

void usb_audio_change(bool state);
void usb_audio_set_mode(bool uac1);
void usb_audio_set_rate(char* rate);
void usb_audio_set_bits(char* bits);

#endif // __USB_AUDIO_H__
