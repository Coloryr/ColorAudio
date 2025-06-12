#ifndef _SOUND_H_
#define _SOUND_H_

#include <alsa/asoundlib.h>

void alsa_init();
void alsa_set(snd_pcm_format_t format, uint16_t channels, uint16_t rate);
void alsa_reset();
int alsa_write(void *buffer, uint16_t frame_len);

#endif