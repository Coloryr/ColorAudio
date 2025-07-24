#ifndef _SOUND_FFT_H_
#define _SOUND_FFT_H_

#include <stdint.h>

extern int32_t *sound_fft_buf;

void fft_check_buffer(uint16_t len);
void fft_fill(uint32_t down);

#endif