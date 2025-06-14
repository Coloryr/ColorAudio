#ifndef FFT_H
#define FFT_H

#ifdef __cplusplus
extern "C"
{
#endif
void fft_run(float *vReal, float *vImag, uint16_t samples);
#ifdef __cplusplus
}
#endif
#endif