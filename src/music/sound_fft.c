#include "sound_fft.h"
#include "sound.h"

#include "lvgl.h"
#include "fft/FFT.h"
#include "view.h"
#include "music_main.h"

#include <stdint.h>
#include <malloc.h>
#include <math.h>

#define POINTS 2048
#define OUT_POINTS 20
#define SQRT2 1.4142135623730951

int32_t *sound_fft_buf;

static uint16_t input_fft_index;
static float input_fft_data[POINTS];
static float input_fft_data_imag[POINTS];
static float freqs[POINTS / 2];
static int freq_bands[] = {50, 69, 94, 129, 176, 241, 331, 453, 620, 850, 1200, 1600, 2200, 3000, 4100, 5600, 7700, 11000, 14000, 20000};

uint16_t fft_now_size;

void fft_check_buffer(uint16_t len)
{
    if (fft_now_size != len)
    {
        if (sound_fft_buf)
        {
            free(sound_fft_buf);
        }
        sound_fft_buf = malloc(sizeof(int32_t) * len);
        fft_now_size = len;
    }
}

void fft_fill(uint32_t down)
{
    bool skip = false;
    if (fft_now_size >= POINTS)
    {
        for (size_t i = 0; i < POINTS; i++)
        {
            input_fft_data[i] = ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
            input_fft_data_imag[i] = input_fft_data[i];
        }
    }
    else
    {
        uint16_t less = POINTS - input_fft_index;
        if (input_fft_index + fft_now_size < POINTS)
        {
            skip = true;
        }
        for (size_t i = 0; i < LV_MIN(less, fft_now_size); i++)
        {
            input_fft_data[input_fft_index] = ((float)sound_fft_buf[i]) / down * POINTS / (POINTS / 2) / SQRT2;
            input_fft_data_imag[input_fft_index] = input_fft_data[input_fft_index];
            input_fft_index++;
        }
    }
    if (skip)
    {
        return;
    }

    input_fft_index = 0;

    fft_run(input_fft_data, input_fft_data_imag, POINTS);

    uint16_t len = POINTS / 2;

    for (uint16_t i = 0; i < len; i++)
    {
        float freq = (float)i * pcm_now_rate / POINTS;
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
            bin_values[bin] = LV_MAX(input_fft_data[j], bin_values[bin]);
            bin_values[bin] += input_fft_data[j];
        }
        bin_values[bin] /= (end_idx - start_idx);
        bin_values[bin] = log10f(bin_values[bin]);
        if (bin_values[bin] < 0)
        {
            bin_values[bin] = 0;
        }
    }

    for (int i = 0; i < OUT_POINTS; i++)
    {
        int bar_len = (int)(bin_values[i] * 20);
        view_set_fft_data(i, bar_len, 0);
    }
}
