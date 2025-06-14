#include "arduinoFFT.h"
#include "FFT.h"
#include "math.h"
#include "utils.h"
#include "main.h"

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

    //for (int i = 0; i < samples; i++)
    //{
        //vReal[i] = vReal[i] / (samples / 2);
    //}
}
