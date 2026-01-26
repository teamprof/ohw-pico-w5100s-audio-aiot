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
#pragma once
#include "arm_math.h"

class AudioModel;

class PreProcessor
{
public:
    PreProcessor(int fft_size);
    virtual ~PreProcessor();

    static PreProcessor *getInstance(void);

    arm_status init(AudioModel *);
    void update_spectrum(const int16_t *raw_input);

private:
    static PreProcessor *_instance;
    static q15_t _audio_buf[];

    const int _fft_size;
    q15_t *_hanning_window;
    arm_rfft_instance_q15 _S_q15;

    int8_t *_spectrogram;
    int32_t _spectrogram_width;
    int32_t _spectrogram_height;
    int32_t _spectrogram_divider;
    float _spectrogram_zero_point;

    void shift_spectrogram(int shift_amount);
    void calculate_spectrum(const q15_t *input, int8_t *output);
};