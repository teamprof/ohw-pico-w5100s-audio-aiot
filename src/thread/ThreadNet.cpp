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

#include <mbed.h>
#include <EventEthernet.h>
#include <utility/w5100.h>

#include "./ThreadNet.h"
#include "../AppContext.h"
#include "../pins.h"
#include "../util/util.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Disable Logging Macro (Release Mode)
// #define DEBUGLOG_DISABLE_LOG
// You can also set default log level by defining macro (default: INFO)
#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE
#include <DebugLog.h> // https://github.com/hideakitai/DebugLog

// #define PIN_ETH_CS 17u   // W5100S-EVB-Pico: nCS = GPIO17
// #define PIN_ETH_INTN 21u // W5100S-EVB-Pico: INTn = GPIO21

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
static byte mac[] = {
    0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

////////////////////////////////////////////////////////////////////////////////////////////
ThreadNet *ThreadNet::_instance = nullptr;

ThreadNet *ThreadNet::getInstance(void)
{
    if (!_instance)
    {
        static ThreadNet instance;
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

#define TASK_NAME "ThreadNet"
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
ThreadNet::ThreadNet() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         _handlerMap()
{
    _instance = this;

    _handlerMap = {
        __EVENT_MAP(ThreadNet, EventNull), // {EventNull, &ThreadNet::handlerEventNull},
    };
}

void ThreadNet::start(void *ctx)
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

#define TASK_NAME "ThreadNet"
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
ThreadNet::ThreadNet() : ardufreertos::ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         _handlerMap()
{
    _instance = this;

    // setup event handlers
    _handlerMap = {
        __EVENT_MAP(ThreadNet, EventNull), // {EventNull, &ThreadNet::handlerEventNull},
    };
}

void ThreadNet::start(void *ctx)
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
// Thread for Mbed RP2040
////////////////////////////////////////////////////////////////////////////////////////////
#define THREAD_QUEUE_SIZE (128 * EVENTS_EVENT_SIZE) // message queue size for app thread

/////////////////////////////////////////////////////////////////////////////
// use static threadQueue instead of heap
static events::EventQueue threadQueue(THREAD_QUEUE_SIZE);
ThreadNet::ThreadNet() : ardumbedos::ThreadBase(&threadQueue),
                         _handlerMap(),
                         //  _inferenceState(InferenceUnknown),
                         _state({0}),
                         _callmebot([](Callmebot::MessageState state)
                                    {
                                        LOG_TRACE("Callmebot state=", state);
                                        auto instance = getInstance();
                                        instance->postEvent(EventApp, AppCallmebotState, (uint32_t)state, 0);
                                        //
                                    })
/////////////////////////////////////////////////////////////////////////////
// threadQueue is dynamically allocate from heap
// ThreadApp::ThreadApp() : ThreadBase(THREAD_QUEUE_SIZE),
//                          _handlerMap()
/////////////////////////////////////////////////////////////////////////////
{
    _handlerMap = {
        __EVENT_MAP(ThreadNet, EventApp),
        __EVENT_MAP(ThreadNet, EventSystem),
        __EVENT_MAP(ThreadNet, EventNull), // {EventNull, &ThreadNet::handlerEventNull},
    };
}

void ThreadNet::start(void *ctx)
{
    LOG_TRACE("core", get_core_num(), ", ctx=(hex)", DebugLogBase::HEX, (uint32_t)ctx);
    ThreadBase::start(ctx);
}

#endif

void ThreadNet::setup(void)
{
    ThreadBase::setup();

    initEth();

    queue()->call_every(std::chrono::seconds(1), [this]()
                        {
                            postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)TIMER_1HZ);
                            //
                        });
}

/////////////////////////////////////////////////////////////////////////////
void ThreadNet::onEthernetEvent(uint8_t ir, uint8_t ir2, uint8_t slir)
{
    LOG_DEBUG("ir=(hex)", DebugLogBase::HEX, ir, ", ir2=(hex)", ir2, ", slir=(hex)", slir);
    auto instance = getInstance();
    EthIR ethIR = {
        .data = {
            .ir = ir,
            .ir2 = ir2,
            .slir = slir,
        }};
    instance->postEvent(EventSystem, SysEthIf, 0, ethIR.word);
}

void ThreadNet::initEth(void)
{
    LOG_DEBUG("Ethernet.init(", PIN_ETH_CS, ", ", PIN_ETH_INTN, ") // W5100S-EVB-Pico: nCS pin = ", PIN_ETH_CS, ", Intn pin = ", PIN_ETH_INTN);
    Ethernet.init(PIN_ETH_CS, PIN_ETH_INTN); // W5100S-EVB-Pico: nCS pin = GPIO17, Intn pin = GPIO21

    // subscribe CONFLICT, UNREACH and PPPTERM interrupts on IR (Interrupt Register)
    // subscribe WOL interrupt on IR2 (Interrupt Register 2)
    // subscribe TIMEOUT, ARP and PING interrupts on SLIR (SOCKET-less Interrupt Register)
    uint8_t ir = IR::CONFLICT | IR::UNREACH | IR::PPPTERM;
    uint8_t ir2 = IR2::WOL;
    uint8_t slir = SLIR::TIMEOUT | SLIR::ARP | SLIR::PING;
    while (Ethernet.begin(mac, ir, ir2, slir, onEthernetEvent) == 0)
    {
        LOG_DEBUG("Failed to configure Ethernet using DHCP");

        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            LOG_ERROR("Ethernet was not found. Sorry, can't run without hardware. :(");
        }
        else if (Ethernet.linkStatus() == LinkOFF)
        {
            LOG_DEBUG("Ethernet cable is not connected");
        }

        rtos::ThisThread::sleep_for(1s);
        // delay(1000);
    }

    LOG_DEBUG("localIP(): ", Ethernet.localIP());
    LOG_DEBUG("dnsServerIP(): ", Ethernet.dnsServerIP());

    static const IPAddress NullIP(0, 0, 0, 0);
    if (Ethernet.localIP() != NullIP)
    {
        _state.netIfUp = true;
        auto ctx = static_cast<AppContext *>(context());
        postEvent(ctx->threadApp, EventApp, AppEthUp);
    }
}

/////////////////////////////////////////////////////////////////////////////
void ThreadNet::onMessage(const Message &msg)
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
__EVENT_FUNC_DEFINITION(ThreadNet, EventApp, msg) // void ThreadNet::handlerEventApp(const Message &msg)
{
    LOG_TRACE("EventApp(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    auto src = static_cast<AppTriggerSource>(msg.iParam);
    switch (src)
    {
    case AppInference:
    {
        auto inferenceState = static_cast<InferenceState>(msg.uParam);
        auto text = getAlertText(inferenceState);
        if (text)
        {
            _callmebot.send(text);
        }
        // rtos::ThisThread::sleep_for(1s);
        break;
    }

    case AppCallmebotState:
    {
        auto callmebotState = static_cast<Callmebot::MessageState>(msg.uParam);
        LOG_TRACE("Callmebot state=", callmebotState);
        _state.callmebotReady = (callmebotState == Callmebot::Sending);
        break;
    }

    default:
        // DBGLOG(Debug, "Unsupported src=%d, uParam=%u, lParam=%lu", src, msg.uParam, msg.lParam);
        LOG_TRACE("Unsupported src=", src, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
        break;
    }
}
__EVENT_FUNC_DEFINITION(ThreadNet, EventSystem, msg) // void ThreadNet::handlerEventSystem(const Message &msg)
{
    // LOG_TRACE("EventSystem(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        handlerSoftwareTimer(msg.lParam);
        break;
    case SysEthIf:
    {
        handlerEthIf(msg.lParam);
        break;
    }
    default:
        LOG_TRACE("unsupported SystemTriggerSource=", src);
        break;
    }
}

// define EventNull handler
__EVENT_FUNC_DEFINITION(ThreadNet, EventNull, msg) // void ThreadNet::handlerEventNull(const Message &msg)
{
    LOG_TRACE("EventNull(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
}
/////////////////////////////////////////////////////////////////////////////

void ThreadNet::handlerSoftwareTimer(uint32_t xTimer)
{
    if (xTimer == TIMER_1HZ)
    {
        // LOG_TRACE("_timer1Hz");
        _callmebot.update();
    }
    else
    {
        LOG_TRACE("unsupported timer handle=0x%04x", (uint32_t)(xTimer));
    }
}

void ThreadNet::handlerEthIf(uint32_t ethIR)
{
    EthIR regs = {
        .word = ethIR,
    };

    auto ir = regs.data.ir;
    auto ir2 = regs.data.ir2;
    auto slir = regs.data.slir;

    ///////////////////////////////////////////////////////////////////////
    // Interrupt Register event
    if (ir & IR::CONFLICT)
    {
        LOG_DEBUG("Ir::CONFLICT");
    }
    if (ir & IR::UNREACH)
    {
        LOG_DEBUG("Ir::UNREACH");
    }
    if (ir & IR::PPPTERM)
    {
        LOG_DEBUG("Ir::PPPTERM");
    }

    ///////////////////////////////////////////////////////////////////////
    // Interrupt Register 2 event
    if (ir2 & IR2::WOL)
    {
        LOG_DEBUG("Ir2::WOL");
    }

    ///////////////////////////////////////////////////////////////////////
    // SOCKET-less Interrupt Register event
    if (slir & SLIR::TIMEOUT)
    {
        LOG_DEBUG("Slir::TIMEOUT");
    }
    if (slir & SLIR::ARP)
    {
        LOG_DEBUG("Slir::ARP");
    }
    if (slir & SLIR::PING)
    {
        LOG_DEBUG("Slir::PING");
    }
}

const char *ThreadNet::getAlertText(InferenceState state)
{
    switch (state)
    {
    case InferenceAlarmOn:
        return "alarm sound detected";
    case InferenceAlarmOff:
        return "no alarm";
    default:
        return nullptr;
    }
}