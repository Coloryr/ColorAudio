#include "arduinoFFT.h"
#include "FFT.h"

ArduinoFFT<float> *FFT;

void fft_run(float *vReal, float *vImag, uint16_t samples)
{
    if (FFT == NULL)
    {
        FFT = new ArduinoFFT<float>(vReal, vImag, samples, 0);
    }
    FFT->windowing(FFTWindow::Hann, FFTDirection::Forward);
    FFT->compute(FFTDirection::Forward);
    FFT->complexToMagnitude();
}
