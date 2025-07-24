#ifndef _SOUND_H_
#define _SOUND_H_

#include <alsa/asoundlib.h>

extern int32_t *sound_buf;

extern uint16_t pcm_now_format;
extern uint16_t pcm_now_channels;
extern uint32_t pcm_now_rate;
extern uint32_t pcm_now_size;

#ifdef __cplusplus
extern "C" {
#endif

float alsa_get_volume();
void alsa_set_volume(float value);
void alsa_reset();
void alsa_init();
void alsa_set(snd_pcm_format_t format, uint16_t channels, uint32_t rate);
void alsa_check_buffer(uint16_t len);
void alsa_clear();
void alsa_ready();
int alsa_write();
int alsa_write_buffer(const void *buffer, size_t samples);
void alsa_set_volume_db(long value);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif