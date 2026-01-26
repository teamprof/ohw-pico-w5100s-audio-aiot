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
#include <stdio.h>
#include "hardware/pio.h"

#include "./audio_const.h"

#define NUM_CHANNELS 1 // total number of channels: 1=mono; 2=stereo
// #define NUM_CHANNELS 2 // total number of channels: 1=mono; 2=stereo

#define DMA_BUFFER_SIZE (AUDIO_FRAME_LEN * NUM_CHANNELS)

namespace pioi2s
{
    typedef struct config_t
    {
        uint32_t fs;       // sampling frequency
        uint8_t bit_depth; // number of bits per channel
        uint8_t din_pin;
        uint8_t clock_pin_base;
    } config_t;

    typedef struct clocks_t
    {
        // Clock computation results
        float bck_pio_hz;

        // PIO divider ratios to obtain the computed clocks above
        uint16_t bck_d;
        uint8_t bck_f;
    } clocks_t;

    // NOTE: Use __attribute__ ((aligned(8))) on this struct or the DMA wrap won't work!
    typedef struct pio_i2s_t
    {
        int32_t *dma_in_ctrl_blocks[2]; // Control blocks MUST have 8-byte alignment.
        uint dma_ch_in_ctrl;
        uint dma_ch_in_data;
        int32_t dma_in_buffer[2][DMA_BUFFER_SIZE];

        PIO pio;
        uint8_t sm_mask;
        uint8_t sm_din;
        uint offset_din;

        config_t config;
    } pio_i2s_t;

    extern const config_t i2s_config_default;

    bool master_in_mono_left_start(const config_t *config, void (*dma_handler)(void), pio_i2s_t *i2s);

} // namespace pioi2s
