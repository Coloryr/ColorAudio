#include <stdint.h>
#include <malloc.h>
#include <math.h>

#include "fft/arduinoFFT.h"
#include "lvgl.h"

#include "sound.h"
#include "ui/ui.h"

#include "sound_fft.h"

#define POINTS (1024 * 4)
#define OUT_POINTS 20
#define SQRT2 1.4142135623730951

int32_t *sound_fft_buf;

static uint16_t input_fft_index;
static float input_fft_data[POINTS];
static float input_fft_data_imag[POINTS];
static float freqs[POINTS / 2];
static int freq_bands[] = {50, 69, 94, 129, 176, 241, 331, 453, 620, 850, 1200, 1600, 2200, 3000, 4100, 5600, 7700, 11000, 14000, 20000};

static ArduinoFFT<float> *fft;

static uint16_t fft_now_size;

static void fft_run(float *vReal, float *vImag, uint16_t samples)
{
    if (fft == NULL)
    {
        fft = new ArduinoFFT<float>(vReal, vImag, samples, 0);
    }
    fft->windowing(FFTWindow::Hann, FFTDirection::Forward);
    fft->compute(FFTDirection::Forward);
    fft->complexToMagnitude();
}

static bool fft_fill_data(uint32_t down, uint32_t count, uint16_t *down_index)
{
    bool skip = false;
    if (pcm_now_rate > 48000)
    {
        *down_index = pcm_now_rate / 48000;

        if (count >= POINTS * *down_index)
        {
            input_fft_index = 0;
            for (size_t i = 0; input_fft_index < POINTS; i += *down_index)
            {
                input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index] =
                    ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
                input_fft_index++;
            }
        }
        else
        {
            for (size_t i = 0; i < count; i += *down_index)
            {
                input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index] =
                    ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
                input_fft_index++;
                if (input_fft_index >= POINTS)
                {
                    break;
                }
            }
        }
    }
    else
    {
        if (count >= POINTS)
        {
            input_fft_index = 0;
            for (size_t i = 0; i < POINTS; i++)
            {
                input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index] =
                    ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
                input_fft_index++;
            }
        }
        else
        {
            for (size_t i = 0; i < count; i++)
            {
                input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index] =
                    ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
                input_fft_index++;
                if (input_fft_index >= POINTS)
                {
                    break;
                }
            }
        }
    }
    if (input_fft_index < POINTS)
    {
        skip = true;
    }
    return skip;
}

void fft_fill_count(uint32_t down, uint32_t count)
{
    uint16_t down_index = 1;
    if (fft_fill_data(down, count, &down_index))
    {
        return;
    }
    input_fft_index = 0;

    fft_run(input_fft_data, input_fft_data_imag, POINTS);

    uint16_t len = POINTS / 2;

    for (uint16_t i = 0; i < len; i++)
    {
        float freq = (float)i * pcm_now_rate / POINTS / down_index;
        freqs[i] = freq;
    }

    uint16_t index = 0;

    float bin_values[OUT_POINTS] = {0};
    int j = 0;
    for (int bin = 0; bin < OUT_POINTS; bin++)
    {
        int start_idx = j;
        for (; j < len; j++)
        {
            if (freqs[j] > freq_bands[bin])
            {
                break;
            }
        }

        int end_idx = j;

        for (int j = start_idx; j < end_idx; j++)
        {
            bin_values[bin] += input_fft_data[j];
        }
        bin_values[bin] /= (end_idx - start_idx);
        bin_values[bin] = log10f(bin_values[bin]);
        if (bin_values[bin] < 0)
        {
            bin_values[bin] = 0;
        }
    }

    view_set_fft_data(OUT_POINTS, bin_values);
}

void fft_check_buffer(uint16_t len)
{
    if (fft_now_size != len)
    {
        if (sound_fft_buf)
        {
            free(sound_fft_buf);
        }
        sound_fft_buf = static_cast<int32_t *>(malloc(sizeof(int32_t) * len * 2));
        fft_now_size = len;
    }
}
