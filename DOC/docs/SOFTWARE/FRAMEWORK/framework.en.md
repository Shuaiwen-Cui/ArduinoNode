# Development Framework

!!! tip "Development Framework"
    For simplicity, this project does not use an RTOS but is developed directly based on the basic model of "initialization" and "loop" of the microcontroller. On this basis, a state machine model is introduced to manage different states. The core of the framework is the `main.cpp` file, which contains the basic logic for initialization and looping. Functions for sensing, communication, storage, etc., are encapsulated for easy invocation as needed.

## Part I Initialization Part

Initialization is responsible for setting up hardware, configuring peripherals, and initializing various modules. In this project, initialization mainly includes the following aspects:

- Hardware configuration: Setting up GPIO, UART, SPI, and other peripheral pins and parameters.
- Module initialization: Initializing sensors, communication modules (such as Wi-Fi, etc.), storage modules, etc.
- System settings: Clock synchronization, system parameter configuration, etc.

```txt
+=============================+        +=============================+
|         GATEWAY NODE        |        |          LEAF NODE          |
+=============================+        +=============================+
| - Init serial               |        | - Init serial               |
| - Init RGB LED              |        | - Init RGB LED              |
| - Init IMU                  |        | - Init IMU                  |
| - Init SD card              |        | - Init SD card              |
| - Init RF module            |        | - Init RF module            |
| - Connect WiFi              |        |                             |
| - Sync time (NTP)           |        |                             |
| - Setup MQTT                |        |                             |
| - RF online check & RTT     |        |                             |
| - RF time sync              | <====> | - RF time sync              |
| - Wait for tasks            |        | - Wait for schedule         |
+-----------------------------+        +-----------------------------+
```

Below is a typical startup process serial output example for the gateway node:

![](printout.png)

!!! tip
    Though each node can be connected to the Internet through Wifi, we only use the gateway node to connect to the Internet in this project for practical considerations. 

## Part II Loop Part

The loop part is the core of the program, responsible for continuously executing tasks such as sensing, processing, and communication. In this project, the loop part mainly manages different states through a state machine by monitoring a series of flags. In the loop part, the leaf nodes continuously listen for messages from the gateway node in the IDLE state, parse the received messages, and execute corresponding operations.

```txt
+===============================================================================================+
|                                     LOOP START                                                |
|                                                                                               |
|  +------+------+-----------+----------+------------------+--------------------+-------+       |
|  | BOOT | IDLE | PREPARING | SAMPLING | RF_COMMUNICATING | WIFI_COMMUNICATING | ERROR |       |
|  +------+------+-----------+----------+------------------+--------------------+-------+       |
|                                                                                               |
|  → Check current state → Execute logic → Repeat                                               |
+===============================================================================================+

```

## Part III Background Callbacks

The background callbacks refer to functions that are automatically triggered when certain events occur during the program's execution. These callback functions are typically used to handle asynchronous events, such as receiving data, timer timeouts, etc. In this project, they are mainly used to listen for messages from the communication module and process them accordingly.

```txt
+============================+                           +============================+
|        FOREGROUND          |                           |        BACKGROUND          |
|  (Main thread execution)   |                           |   (Interrupts / Callbacks) |
+============================+                           +============================+
|                            |                           |                            |
|  setup():                  |                           |  - RF receive interrupt    |
|  - Init serial / LED       |                           |  - MQTT message callback   |
|  - Init IMU / SD / RF      |                           |  - Timer / millis() event  |
|  - WiFi + NTP + MQTT       |                           |                            |
|                            |                           |                            |
|  loop():                   |                           |                            |
|  - Check node state        | <--------- flags -------- |  - Set flags / schedule    |
|  - Execute logic           | --------> state change -> |  - Modify state machine    |
|  - Maintain MQTT (only GW) |                           |                            |
|  - Transition states       |                           |                            |
|                            |                           |                            |
+============================+                           +============================+

```