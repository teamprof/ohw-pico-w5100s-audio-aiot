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
#include <map>
#include <SimpleKalmanFilter.h>
#include "../ArduProfApp.h"
#include "../AppEvent.h"
#include "../peripheral/LedGreen.h"

#if defined ARDUPROF_FREERTOS
class ThreadApp : public ardufreertos::ThreadBase
#elif defined ARDUPROF_MBED
class ThreadApp : public ardumbedos::ThreadBase
#endif
{
public:
    typedef struct _ThreadState
    {
        uint32_t netIfUp : 1; // network interface is up
        uint32_t alarmOn : 1; // alarm is on
    } ThreadState;

    ThreadApp();

    static ThreadApp *getInstance(void);

    virtual void start(void *);
    virtual void onMessage(const Message &msg);

protected:
    typedef void (ThreadApp::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> _handlerMap;

private:
    static ThreadApp *_instance;
    LedGreen _ledGreen;
    SimpleKalmanFilter _kf; // SimpleKalmanFilter(e_mea, e_est, q);
    ThreadState _state;

    virtual void setup(void);
    void handlerEthUp(void);
    void handlerEthDn(void);
    void handlerInference(uint32_t prediction);

    ///////////////////////////////////////////////////////////////////////
    // declare event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventApp)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};
