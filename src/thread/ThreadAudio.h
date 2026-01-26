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
#include <mbed.h>
#include <rtos.h>
#include "../ArduProfApp.h"

class AudioModel;
class PreProcessor;

#if defined ARDUPROF_FREERTOS
class ThreadAudio : public ardufreertos::ThreadBase
#elif defined ARDUPROF_MBED
class ThreadAudio : public ardumbedos::ThreadBase
#endif
{
public:
    static const uint32_t EVENT_I2S_DMA = (1 << 1);
    static const uint32_t EVENT_PDM_DMA = (1 << 2);

    typedef void (*DmaCallback)(void);

    ThreadAudio();

    static ThreadAudio *getInstance(void);
    virtual void start(void *);
    virtual void onMessage(const Message &msg) {}
    virtual void run(void);

private:
    static ThreadAudio *_instance;
    AudioModel *_model;
    PreProcessor *_preprocessor;

    rtos::EventFlags _eventFlags;
    DmaCallback _dmaCallback;

    virtual void setup(void);

    static int16_t *get_buffer_ptr(void);
    static size_t get_buffer_size(void);
    static void dma_i2s_in_handler(void);
    bool start_i2s_in(DmaCallback callback);
};
