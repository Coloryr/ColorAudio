#ifndef _SOUND_FFT_H_
#define _SOUND_FFT_H_

#include <stdint.h>

extern int32_t *sound_fft_buf;

#ifdef __cplusplus
extern "C" {
#endif

void fft_check_buffer(uint16_t len);
void fft_fill_count(uint32_t down, uint32_t count);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif