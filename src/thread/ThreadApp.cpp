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
#include "./ThreadApp.h"
#include "../AppContext.h"
#include "../util/util.h"

#define THRESHOLD_INFERENCE 0.3 //

// Define the filter with parameters suited for a 0.0 to 1.0 range:
// e_mea = 0.01, e_est = 0.01, q = 0.0005 (Tune these values!)
// SimpleKalmanFilter sensorKalmanFilter(0.01, 0.01, 0.0005);
#define KF_E_MEA 0.01
#define KF_E_EST 0.01
#define KF_Q 0.0005

////////////////////////////////////////////////////////////////////////////////////////////
ThreadApp *ThreadApp::_instance = nullptr;

ThreadApp *ThreadApp::getInstance(void)
{
    if (!_instance)
    {
        static ThreadApp instance;
        _instance = &instance;
    }
    return _instance;
}

#if defined ARDUPROF_FREERTOS && defined ARDUINO_ARCH_RP2040
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for FreeRTOS RP2040/RP2350
////////////////////////////////////////////////////////////////////////////////////////////

static constexpr UBaseType_t uxCoreAffinityMask = ((1 << 0)); // task only run on core 0
// static constexpr UBaseType_t uxCoreAffinityMask = ((1 << 1)); // task only run on core 1
// static constexpr uxCoreAffinityMask = ( ( 1 << 0 ) | ( 1 << 2 ) );  // e.g. task can only run on core 0 and core 2

#define TASK_NAME "ThreadApp"
#define TASK_STACK_SIZE (4096 / sizeof(StackType_t))
#define TASK_PRIORITY 6   // Priority, (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
#define TASK_QUEUE_SIZE 8 // message queue size for app task
static_assert(TASK_PRIORITY <= configMAX_PRIORITIES, "TASK_PRIORITY exceeds configMAX_PRIORITIES");

#define TASK_INIT_NAME "taskDelayInit"
#define TASK_INIT_STACK_SIZE (4096 / sizeof(StackType_t))
#define TASK_INIT_PRIORITY 0
static_assert(TASK_INIT_PRIORITY <= configMAX_PRIORITIES, "TASK_INIT_PRIORITY exceeds configMAX_PRIORITIES");

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

///////////////////////////////////////////////////////////////////////
ThreadApp::ThreadApp() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         _handlerMap()
{
    _instance = this;

    _handlerMap = {
        __EVENT_MAP(ThreadApp, EventNull), // {EventNull, &ThreadApp::handlerEventNull},
    };
}

void ThreadApp::start(void *ctx)
{
    LOG_TRACE("core", get_core_num());
    configASSERT(ctx);
    _context = ctx;

    _taskHandle = xTaskCreateStatic(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer);
    configASSERT(_taskHandle);
    vTaskCoreAffinitySet(_taskHandle, uxCoreAffinityMask); // Set the core affinity mask for the task, i.e. set task on running core
}

#elif defined ARDUPROF_FREERTOS && defined ESP_PLATFORM
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for ESP32
////////////////////////////////////////////////////////////////////////////////////////////

// #define RUNNING_CORE 0 // dedicate core 0 for Thread
// #define RUNNING_CORE 1 // dedicate core 1 for Thread
#define RUNNING_CORE ARDUINO_RUNNING_CORE

#define TASK_NAME "ThreadApp"
#define TASK_STACK_SIZE (4096 / sizeof(StackType_t))
#define TASK_PRIORITY 6   // Priority, (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
#define TASK_QUEUE_SIZE 8 // message queue size for app task
static_assert(TASK_PRIORITY <= configMAX_PRIORITIES, "TASK_PRIORITY exceeds configMAX_PRIORITIES");

#define TASK_INIT_NAME "taskDelayInit"
#define TASK_INIT_STACK_SIZE (4096 / sizeof(StackType_t))
#define TASK_INIT_PRIORITY 0
static_assert(TASK_INIT_PRIORITY <= configMAX_PRIORITIES, "TASK_INIT_PRIORITY exceeds configMAX_PRIORITIES");

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadApp::ThreadApp() : ardufreertos::ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         _handlerMap()
{
    _instance = this;

    // setup event handlers
    _handlerMap = {
        __EVENT_MAP(ThreadApp, EventNull), // {EventNull, &ThreadApp::handlerEventNull},
    };
}

void ThreadApp::start(void *ctx)
{
    // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    ThreadBase::start(ctx);

    _taskHandle = xTaskCreateStaticPinnedToCore(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer,
        RUNNING_CORE);
}

#elif defined ARDUPROF_MBED && defined ARDUINO_ARCH_MBED_RP2040
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for MBed RP2040
////////////////////////////////////////////////////////////////////////////////////////////
#define THREAD_QUEUE_SIZE (64 * EVENTS_EVENT_SIZE) // message queue size for app thread

/////////////////////////////////////////////////////////////////////////////
// use static threadQueue instead of heap
static events::EventQueue threadQueue(THREAD_QUEUE_SIZE);
ThreadApp::ThreadApp() : ardumbedos::ThreadBase(&threadQueue),
                         _handlerMap(),
                         _ledGreen(),
                         _kf(KF_E_MEA, KF_E_EST, KF_Q),
                         _state({0})
/////////////////////////////////////////////////////////////////////////////
// threadQueue is dynamically allocate from heap
// ThreadApp::ThreadApp() : ThreadBase(THREAD_QUEUE_SIZE),
//                          _handlerMap()
/////////////////////////////////////////////////////////////////////////////
{
    _handlerMap = {
        __EVENT_MAP(ThreadApp, EventApp),
        __EVENT_MAP(ThreadApp, EventNull), // {EventNull, &ThreadApp::handlerEventNull},
    };
}

void ThreadApp::start(void *ctx)
{
    LOG_TRACE("core", get_core_num(), ", ctx=(hex)", DebugLogBase::HEX, (uint32_t)ctx);
    ThreadBase::start(ctx);
}

#endif

void ThreadApp::setup(void)
{
    ThreadBase::setup();

    auto ctx = reinterpret_cast<AppContext *>(context());
    if (ctx->threadNet)
    {
        ctx->threadNet->start(ctx);
    }
}

/////////////////////////////////////////////////////////////////////////////
void ThreadApp::onMessage(const Message &msg)
{
    // LOG_TRACE("event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    auto func = _handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        LOG_TRACE("Unsupported event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    }
}

/////////////////////////////////////////////////////////////////////////////

__EVENT_FUNC_DEFINITION(ThreadApp, EventApp, msg) // void ThreadApp::handlerEventApp(const Message &msg)
{
    auto src = static_cast<AppTriggerSource>(msg.iParam);
    switch (src)
    {
    case AppInference:
        handlerInference(msg.lParam);
        break;
    case AppEthUp:
        handlerEthUp();
        break;
    case AppEthDn:
        handlerEthDn();
        break;
    default:
        // DBGLOG(Debug, "Unsupported src=%d, uParam=%u, lParam=%lu", src, msg.uParam, msg.lParam);
        LOG_TRACE("Unsupported src=", src, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
        break;
    }
}

// define EventNull handler
__EVENT_FUNC_DEFINITION(ThreadApp, EventNull, msg) // void ThreadApp::handlerEventNull(const Message &msg)
{
    LOG_TRACE("EventNull(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
}
/////////////////////////////////////////////////////////////////////////////

void ThreadApp::handlerEthUp(void)
{
    LOG_TRACE("AppEthUp");

    ///////////////////////////////////////////////////////////////////////////
    // blink on-board LED three times
    static const int BLINK_TIMES = 5;
    for (int i = 0; i < BLINK_TIMES; i++)
    {
        _ledGreen.on();
        rtos::ThisThread::sleep_for(200ms);
        _ledGreen.off();
        rtos::ThisThread::sleep_for(200ms);
    }
    ///////////////////////////////////////////////////////////////////////////

    // launch ThreadAudio once eth is up
    auto ctx = reinterpret_cast<AppContext *>(context());
    if (ctx->threadAudio)
    {
        ctx->threadAudio->start(ctx);
    }
}

void ThreadApp::handlerEthDn(void)
{
    LOG_TRACE("AppEthDn");
}

void ThreadApp::handlerInference(uint32_t prediction)
{
    float measured_value = uint32_to_float(prediction);
    float estimated_value = _kf.updateEstimate(measured_value);
    bool alarmState = (estimated_value >= THRESHOLD_INFERENCE);
    // LOG_TRACE("measured_value=", (uint32_t)(measured_value * 100.0));
    if (_state.alarmOn == alarmState)
    {
        return;
    }

    _state.alarmOn = alarmState;
    LOG_TRACE("_state.alarmOn=", (uint32_t)_state.alarmOn, ", measured_value=", measured_value, ", estimated_value=", estimated_value);

    auto ctx = reinterpret_cast<AppContext *>(context());
    if (alarmState)
    {
        _ledGreen.on();
        postEvent(ctx->threadNet, EventApp, AppInference, InferenceAlarmOn);
    }
    else
    {
        _ledGreen.off();
        postEvent(ctx->threadNet, EventApp, AppInference, InferenceAlarmOff);
    }
}