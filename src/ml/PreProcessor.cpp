/* Copyright 2026 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "PreProcessor.h"
#include "audio_model.h"
#include "../audio/audio_const.h"
#include "../AppDef.h"

#define SCALE_FACTOR 64

////////////////////////////////////////////////////////////////////////////////////////////
PreProcessor *PreProcessor::_instance = nullptr;
ML_DATA q15_t PreProcessor::_audio_buf[AUDIO_FRAME_LEN + AUDIO_FRAME_STEP];

////////////////////////////////////////////////////////////////////////////////////////////
PreProcessor::PreProcessor(int fft_size) : _fft_size(fft_size),
                                           _hanning_window(NULL),
                                           _S_q15({0}),
                                           _spectrogram(nullptr),
                                           _spectrogram_width(0),
                                           _spectrogram_height(0),
                                           _spectrogram_divider(0),
                                           _spectrogram_zero_point(0.0)
{
}

PreProcessor::~PreProcessor()
{
    if (_hanning_window != NULL)
    {
        delete[] _hanning_window;
        _hanning_window = NULL;
    }
}

PreProcessor *PreProcessor::getInstance(void)
{
    if (!_instance)
    {
        static ML_DATA PreProcessor preprocessor(AUDIO_FFT_LEN);
        _instance = &preprocessor;
    }
    return _instance;
}

arm_status PreProcessor::init(AudioModel *model)
{
    if (!model)
    {
        return ARM_MATH_ARGUMENT_ERROR;
    }

    if (!_hanning_window)
    {
        _hanning_window = new q15_t[_fft_size];
    }
    if (!_hanning_window)
    {
        return ARM_MATH_LENGTH_ERROR;
    }
    for (int i = 0; i < _fft_size; i++)
    {
        float32_t f = 0.5 * (1.0 - arm_cos_f32(2 * PI * i / _fft_size));
        arm_float_to_q15(&f, &_hanning_window[i], 1);
    }

    _spectrogram = (int8_t *)model->input_data();
    _spectrogram_width = model->input_width();
    _spectrogram_height = model->input_height();
    _spectrogram_divider = SCALE_FACTOR * model->input_scale();
    _spectrogram_zero_point = model->input_zero_point();
    MicroPrintf("_spectrogram=%x, _spectrogram_width=%d, _spectrogram_height=%d",
                (uint32_t)_spectrogram, _spectrogram_width, _spectrogram_height);
    MicroPrintf("_spectrogram_divider=%d, _spectrogram_zero_point=%d",
                _spectrogram_divider, _spectrogram_zero_point);

    return arm_rfft_init_q15(&_S_q15, _fft_size, 0, 1);
}

void PreProcessor::shift_spectrogram(int shift_amount)
{
    int8_t *spectrogram = _spectrogram;
    int32_t spectrogram_width = _spectrogram_width;   // kSpectrogramWidth=124
    int32_t spectrogram_height = _spectrogram_height; // kSpectrogramHeight=129 (_fft_size / 2 + 1)

    memmove(spectrogram,
            spectrogram + (spectrogram_height * shift_amount),
            spectrogram_height * (spectrogram_width - shift_amount) * sizeof(spectrogram[0]));
}

void PreProcessor::update_spectrum(const int16_t *raw_input)
{
    q15_t *input = _audio_buf;
    int8_t *output = _spectrogram;

    memmove(input, &input[AUDIO_FRAME_LEN], AUDIO_FRAME_STEP);
    arm_shift_q15(raw_input, AUDIO_INPUT_SHIFT, &input[AUDIO_FRAME_STEP], AUDIO_FRAME_LEN);
    shift_spectrogram(SPECTROGRAM_SHIFT);

    for (int i = 0; i < SPECTROGRAM_SHIFT; i++)
    {
        calculate_spectrum(
            &input[i * AUDIO_FRAME_STEP],
            output + (_spectrogram_height * (_spectrogram_width - SPECTROGRAM_SHIFT + i)));
    }
}

void PreProcessor::calculate_spectrum(const q15_t *input, int8_t *output)
{
    q15_t windowed_input[_fft_size];
    q15_t fft_q15[_fft_size * 2];
    q15_t fft_mag_q15[_fft_size / 2 + 1];

    // apply the DSP pipeline: Hanning Window + FFT
    arm_mult_q15(_hanning_window, input, windowed_input, _fft_size);
    arm_rfft_q15(&_S_q15, windowed_input, fft_q15);
    arm_cmplx_mag_q15(fft_q15, fft_mag_q15, (_fft_size / 2) + 1);

    int8_t *dst = output;
    for (int j = 0; j < ((_fft_size / 2) + 1); j++)
    {
        *dst++ = __SSAT((fft_mag_q15[j] / _spectrogram_divider) + _spectrogram_zero_point, 8);
    }
}
