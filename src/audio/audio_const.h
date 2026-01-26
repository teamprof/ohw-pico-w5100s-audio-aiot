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

////////////////////////////////////////////////////////////////////////////////////////////
#define AUDIO_SAMPLING_RATE 16000 // sampling frequency = 16kHz
#define AUDIO_CHANNEL_MONO 1
#define AUDIO_CHANNEL_STEREO 2

////////////////////////////////////////////////////////////////////////////////////////////
#define AUDIO_FRAME_LEN 512  // audio frame length
#define AUDIO_FRAME_STEP 128 // stride
#define AUDIO_INPUT_SHIFT 0  // number of bits shift on audio_buffer for arm_shift_q15

////////////////////////////////////////////////////////////////////////////////////////////
#define AUDIO_FFT_LEN 256

#define SPECTROGRAM_SHIFT (AUDIO_FRAME_LEN / AUDIO_FRAME_STEP)

#define I2S_FRAME_SIZE (AUDIO_SAMPLING_RATE * 32 / 1000) // 32ms
