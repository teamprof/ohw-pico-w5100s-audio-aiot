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

#define PICO_AUDIO_KIT_V1_0_0 // for ohw-pico-w5100s-audio-aiot

///////////////////////////////////////////////////////////////////////////////
// Pico Audio Kit: for W5100S-PICO_EVB/Pi Pico (RP2040)
#ifdef PICO_AUDIO_KIT_V1_0_0

#define PIN_DEBUG_USB PIN_GPIO6 // comment out to disable toggle IO during USB callback
#define PIN_DEBUG_DMA PIN_GPIO7 // comment out to disable toggle IO during DMA callback
// #define PIN_DEBUG_AUDIO_TASK PIN_GPIO8 // comment out to disable toggle IO in audio_task

#define PIN_TX0 0u
#define PIN_RX0 1u
#define PIN_I2C1_SDA 2u
#define PIN_I2C1_SCL 3u
#define PIN_TX1 4u
#define PIN_RX1 5u
#define PIN_GPIO6 6u // for debug USB
#define PIN_GPIO7 7u // for debug DMA
// #define PIN_GPIO8 8
// #define PIN_I2S_DOUT 8
#define PIN_I2S_DI 9u
#define PIN_I2S_BCLK 10u
#define PIN_I2S_WS 11u

#define PIN_ETH_CS 17u   // W5100S-EVB-Pico: nCS = GPIO17
#define PIN_ETH_INTN 21u // W5100S-EVB-Pico: INTn = GPIO21

#define PIN_LED 25 // PICO_DEFAULT_LED_PIN: on-board LED (Green)

#endif // PICO_AUDIO_KIT_V1_0_0
