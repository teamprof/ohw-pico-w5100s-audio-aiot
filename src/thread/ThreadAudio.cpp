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
#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "./ThreadAudio.h"
#include "../audio/i2s.h"

#include "../AppContext.h"
#include "../AppEvent.h"
#include "../AppDef.h"
#include "../pins.h"
#include "../util/util.h"
#include "../thread/ThreadApp.h"

#include "../ml/PreProcessor.h"
#include "../ml/audio_model.h"

#define ASSERT_DMA_BUFFER_ALIGN // assert dma buffer 8-byte aligned

// select microphone digital gain
#define MIC_GAIN MIC_GAIN_X16

#define MIC_GAIN_X1 0
#define MIC_GAIN_X2 1
#define MIC_GAIN_X4 2
#define MIC_GAIN_X8 3
#define MIC_GAIN_X16 4
#define MIC_GAIN_X32 5

////////////////////////////////////////////////////////////////////////////////////////////
ThreadAudio *ThreadAudio::_instance = nullptr;

static __ALIGNED(8) pioi2s::pio_i2s_t _i2s;
static_assert(alignof(_i2s) == 8, "Alignment of _i2s must be equal to 8 bytes");

static ML_DATA int16_t _audio_buffer[AUDIO_FRAME_LEN];

////////////////////////////////////////////////////////////////////////////////////////////
int16_t *ThreadAudio::get_buffer_ptr(void)
{
    return _audio_buffer;
}

size_t ThreadAudio::get_buffer_size(void)
{
    return sizeof(_audio_buffer);
}

void ThreadAudio::dma_i2s_in_handler(void)
{
#ifdef PIN_DEBUG_DMA
    gpio_xor_mask(1u << PIN_DEBUG_DMA);
#endif

    int32_t *src = *(int32_t **)dma_hw->ch[_i2s.dma_ch_in_ctrl].read_addr;
    int16_t *dst = get_buffer_ptr();

    // convert MSB 24-bit to 16-bit audio data with digital gain
    for (int i = 0; i < DMA_BUFFER_SIZE; i += NUM_CHANNELS)
    {
        *dst++ = (int16_t)(*src >> (16 - MIC_GAIN));
        src += NUM_CHANNELS;
    }

    dma_hw->ints0 = 1u << _i2s.dma_ch_in_data; // clear the IRQ

    auto inst = getInstance();
    if (inst->_dmaCallback)
    {
        (*inst->_dmaCallback)();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
ThreadAudio *ThreadAudio::getInstance(void)
{
    if (!_instance)
    {
        static ThreadAudio instance;
        _instance = &instance;
    }
    return _instance;
}

#if defined ARDUPROF_MBED && defined ARDUINO_ARCH_MBED_RP2040
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for Mbed RP2040
////////////////////////////////////////////////////////////////////////////////////////////
ThreadAudio::ThreadAudio() : ardumbedos::ThreadBase(nullptr, osPriorityRealtime), // no queue for audio thread
                             _model(nullptr),
                             _preprocessor(nullptr),
                             _eventFlags(),
                             _dmaCallback([]()
                                          {
                                              // signal audio task about I2S DMA IRQ
                                              getInstance()->_eventFlags.set(EVENT_I2S_DMA);
                                              //
                                          })
{
}

void ThreadAudio::start(void *ctx)
{
    LOG_TRACE("core", get_core_num(), ", ctx=(hex)", DebugLogBase::HEX, (uint32_t)ctx);

    ThreadBase::start(ctx);
    _thread.start(mbed::callback(this, &ThreadAudio::run));
}
#endif

void ThreadAudio::setup(void)
{
    ThreadBase::setup();

    auto model = AudioModel::getInstance();
    auto preprocessor = PreProcessor::getInstance();

    if ((model->init() == kTfLiteOk) && (preprocessor->init(model) == ARM_MATH_SUCCESS))
    {
        _model = model;
        _preprocessor = preprocessor;

        LOG_TRACE("model->input_width()=", model->input_width(), ", ->input_height()=", model->input_height());
        LOG_TRACE("kSpectrogramWidth=", kSpectrogramWidth, ", kSpectrogramHeight=", kSpectrogramHeight);
    }
    else
    {
        osThreadTerminate(osThreadGetId()); // Terminates the current thread
    }
}

void ThreadAudio::run(void)
{
    setup();

    auto ctx = reinterpret_cast<AppContext *>(context());
    auto thread = reinterpret_cast<ThreadApp *>(ctx->threadApp);
    assert(thread);

    int16_t *raw_buffer_ptr = get_buffer_ptr(); // audio raw data

#if NUM_CHANNELS == 1
    auto bInit = pioi2s::master_in_mono_left_start(&pioi2s::i2s_config_default, &ThreadAudio::dma_i2s_in_handler, &_i2s);
#else
#error "Unsupported NUM_CHANNELS " STR(NUM_CHANNELS)
#endif
    if (!bInit)
    {
        LOG_TRACE("start_i2s_in() failed!");
        osThreadTerminate(osThreadGetId()); // Terminates the current thread
    }

    while (true)
    {
        auto flags = _eventFlags.wait_any(EVENT_I2S_DMA | EVENT_PDM_DMA);
        if (flags & (EVENT_I2S_DMA | EVENT_PDM_DMA))
        {
#ifdef PIN_DEBUG_AUDIO_TASK
            gpio_xor_mask(1u << PIN_DEBUG_AUDIO_TASK);
#endif

#ifdef ASSERT_DMA_BUFFER_ALIGN
            int32_t *src = *(int32_t **)dma_hw->ch[_i2s.dma_ch_in_ctrl].read_addr;
            if ((src != _i2s.dma_in_buffer[0]) && (src != _i2s.dma_in_buffer[1]))
            {
                LOG_TRACE("Error: src=", (uint32_t)src,
                          ", _i2s.dma_in_buffer[0]=", (uint32_t)(_i2s.dma_in_buffer[0]),
                          ", _i2s.dma_in_buffer[1]=", (uint32_t)(_i2s.dma_in_buffer[1]));
                panic(STR(ASSERT_DMA_BUFFER_ALIGN));
            }
#endif // ASSERT_DMA_BUFFER_ALIGN

            _preprocessor->update_spectrum(raw_buffer_ptr);
            float prediction = _model->inference();
            thread->postEvent(EventApp, AppInference, 0, float_to_uint32(prediction));
        }
    }
}
