# Pi Pico Fire alarm sound detector running on W5100S Pico EVB
Integrating audio machine learning on Arduino platforms running Pico Mbed OS presents significant challenges, primarily due to the reliance on external audio libraries such as CMSIS-DSP, and TensorFlow Lite for Microcontrollers (TFLM). These libraries, while powerful, introduce complexities in compatibility and resource management, as they are not natively aligned with Arduino’s lightweight abstraction layers or the constrained runtime environment of Mbed OS.
The integrated platform offers a powerful advantage by seamlessly connecting advanced audio and IoT capabilities with the vast ecosystem of the Arduino community, enabling developers to extend thousands of features with minimal effort.  

Main features:
- W5100S Pico EVB (RP2040) running with Arduino Pico Mbed OS
- Integrate TensorFlow Lite micro
- Integrate CMSIS-DSP + CMSIS_6 Core (note: existing Arduino_CMSIS-DSP does NOT support RP2040 CM0+ MCU)
- Integrate Kalman filter on ML post-processing
- Support I2S 24-bit microphone
- Support Wiznet W5100S Ethernet connectivity
- Detect fire alarm sound and texting user via WhatsApp

This project demonstrates how a W5100S Pico EVB, paired with an INMP441 microphone, can detect fire alarm sounds and send alert messages to the user via WhatsApp using an Ethernet connection.

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/arduino-pico-w5100s-alarm-sound-detector/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 38px !important;width: 128px !important;" ></a>


### System diagram
[![system-diagram](/doc/image/system-diagram.png)](https://github.com/teamprof/arduino-pico-w5100s-alarm-sound-detector/blob/main/image/system-diagram.png)


---
## Hardware
The following components are required for this project:
- ohw-pico-audio-kit PCB (available to purchase on ...)
- W5100S Pico EVB
- TDK InvenSense INMP441
- PC with Arduino v2.3.6+ IDE and Arduino libraries installed (see Software setup section)
- mobile to generate fire alarm sound and receive WhatsApp message

We will appreciate if you can purchase ohw-pico-audio-kit PCB (full package) via PCBWay

---
## Device Block Diagram
```
    ┌───────────────────┐
    │  W5100S Pico EVB  │
    ├──────────┬────────┤     
    │  RP2040  │ W5100S │────► Ethernet 
    └─────▲────┴────────┘     
          |(I2S)         
    ┌─────┴─────┐  
    │  INMP441  │   
    └───────────┘  
```

### demo system photo


---
## Software setup

---
### Build firmware


---
### System operation
When the device boots, it launches ThreadNet to initialize the Wiznet W5100S Ethernet controller. Once the Ethernet network is established, ThreadNet signals ThreadApp with an EthUp event. Upon receiving this event, ThreadApp blinks the on-board LED three times and then launches ThreadAudio.
ThreadAudio initializes the INMP441 I²S microphone and starts the audio inference engine. The inference results are continuously sent from ThreadAudio to ThreadApp. ThreadApp evaluates these results, and if an alarm condition is detected, it instructs ThreadNet to send an alert message via WhatsApp.


## Activity diagram: 
```
                     ThreadAudio               ThreadApp                 ThreadNet  
                          |                        |                         |  
                          |                        |    launch ThreadNet     |  
                          |                        | ----------------------> |  
                          |                        |                         |  
                          |                        |    Ethernet ready       |  
                          |   launch ThreadAudio   | <---------------------- |  
                    +---- | <--------------------- |                         |  
    sound inference |     |                        |                         |  
                    +---> |                        |                         |  
                          |                        |                         |  
                          |                        |                         |  
   fire alarm sound       |                        |                         |  
------------------------> |  alarm sound detected  |                         |  
                          |----------------------> |        alarm on         |  callmebot to WhatsApp  
                          |                        | ----------------------> |  "fire alarm sound on"
                          |                        |                         | ------------------------>
                          |                        |                         |  
                          |                        |                         |  
                          |                        |                         |  
    no alarm sound        |                        |                         |  
------------------------> |    alarm sound off     |                         |  
                          |----------------------> |        alarm off        |  callmebot to WhatsApp
                          |                        | ----------------------> |  "fire alarm sound off"
                          |                        |                         | ------------------------>
                          |                        |                         |  
```
---

## Demo



---
### Debug message
At boot, the system checks for USB CDC availability within "INIT_DEBUG_PORT_TIMEOUT" milliseconds; if detected, all debug messages are routed through USB CDC. If USB CDC is not available within that period, the system automatically falls back to UART0 to ensure debug output remains accessible. Debug port initialization code is available in "initDebugPort()".

Developers can enable or disable logging by modifying the code in "initDebugPort()"
Debug is disabled by calling "LOG_SET_LEVEL(DebugLogLevel::LVL_NONE)" in "initDebugPort()"
Enable trace debug by calling "LOG_SET_LEVEL(DebugLogLevel::LVL_TRACE)" in "initDebugPort()"

Example of ohw-pico-w5100s-audio-aiot.ino
```
static void initDebugPort(void)
{
  ...

  LOG_SET_LEVEL(DebugLogLevel::LVL_TRACE);    // enable debug log
  // LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);  // disable debug log

  ...
}
```

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2026 teamprof.net@gmail.com. All rights reserved.








