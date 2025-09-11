# PROGRAMMING

## 1 PREPARATION

### 1.1 Install VSCode

<div class="grid cards" markdown>

-   :material-microsoft-visual-studio-code:{ .lg .middle } __VSCode__

    ---

    [:octicons-arrow-right-24: <a href="https://code.visualstudio.com/download" target="_blank"> Download Link </a>](#)

</div>

### 1.2 Install PlatformIO

![](platformio.png)

(1) Open VSCode Extension Manager

(2) Search for official PlatformIO IDE extension

(3) Install PlatformIO IDE.

### 1.3 Install Serial Monitor

![](serial-monitor.png)

(1) Open VSCode Extension Manager

(2) Search for official Serial Monitor extension

(3) Install Serial Monitor extension.

### 1.4 Download Project Code

<div class="grid cards" markdown>

-   :material-file:{ .lg .middle } __GitHub__

    ---

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/ArduinoNode/blob/main/CODE.zip" target="_blank"> Download Link </a>](#)

</div>

!!! tip
    Simply click the download button to download the project code zip file.

After downloading, extract the files to your chosen directory.

## 2 PROGRAMMING

### 2.1 Open Project in VSCode

(1) Launch VSCode.

(2) Open the folder where you extracted the project code.

(3) You should see the project structure in the Explorer panel.

### 2.2 Modify Configuration

You need to modify the `src/config.hpp` file to set the appropriate information for your node. Open the `src/config.hpp` file and locate the following code segment:


```cpp
#pragma once
#include <Arduino.h>

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
#define NODE_ID 1 // for LEAFNODE: 1, 2, 3, 4, 5, 6,
// #define NODE_ID 2
// #define NODE_ID 3
// #define NODE_ID 4
// #define NODE_ID 5
// #define NODE_ID 6

#define NUM_NODES 6 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "Shaun's Iphone"
#define WIFI_PASSWORD "cshw0918"

/* MQTT Configurations */
// #define MQTT_CLIENT_ID      "GATEWAY"
#define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"
// #define MQTT_CLIENT_ID      "LEAFNODE3"
// #define MQTT_CLIENT_ID      "LEAFNODE4"
// #define MQTT_CLIENT_ID      "LEAFNODE5"
// #define MQTT_CLIENT_ID      "LEAFNODE6"

#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"

// Sensing Variables 
extern uint64_t sensing_scheduled_start_ms; // Scheduled sensing start time (Unix ms)
extern uint64_t sensing_scheduled_end_ms;   // Scheduled sensing end time (Unix ms)
extern uint32_t sensing_rate_hz;            // Sensing rate in Hz
extern uint32_t sensing_duration_s;         // Sensing duration in seconds
extern uint32_t default_sensing_rate_hz;   // Default sensing rate in Hz
extern uint32_t default_sensing_duration_s; // Default sensing duration in seconds
extern uint16_t parsed_freq;                // Parsed frequency from command
extern uint16_t parsed_duration;            // Parsed duration from command
extern float cali_scale_x; // Calibration scale for X-axis
extern float cali_scale_y; // Calibration scale for Y-axis
extern float cali_scale_z; // Calibration scale for Z-axis

/* Serial Configurations */
// #define DATA_PRINTOUT // Enable data printout to Serial

// === Function Declaration ===
void print_node_config();
```

Now you need to modify the configuration file for each node before uploading the code. Make the following changes:

For the main node (GATEWAY):

- Uncomment `#define GATEWAY` and comment out `#define LEAFNODE`.

- Set `NODE_ID` to 100 and comment out other `NODE_ID`s.

- Set `MQTT_CLIENT_ID` to `GATEWAY` and comment out other `MQTT_CLIENT_ID`s.

For the sensor nodes (LEAFNODE):

- Uncomment `#define LEAFNODE` and comment out `#define GATEWAY`.

- Set `NODE_ID` to one of the numbers from 1 to 6, and comment out other `NODE_ID`s.

- Set `MQTT_CLIENT_ID` to one of `LEAFNODE1` to `LEAFNODE6`, and comment out other `MQTT_CLIENT_ID`s.

Total Number of Nodes:

- Set `NUM_NODES` to the total number of nodes in the network, e.g., 6.

### 2.3 Compile and Upload Code

Follow the steps below to compile and upload the code:

![](icons.jpg)

1. Ensure that the development board is connected to the computer via a Type-C cable.
2. In the PlatformIO toolbar at the bottom of VSCode, click the "Build" button to compile the code.
3. Once the compilation is complete, click the "Upload" button to upload the code to the development board.

### 2.4 View Serial Output

You can use PlatformIO’s built-in serial monitor to view the output from the development board. Click the "Serial Monitor" button at the bottom, select the correct COM port (usually COM3 or a similar name), set the baud rate to 115200, and then click "Connect" to view the output.

!!! warning
    Only one program can access the serial port at a time. Make sure no other programs are occupying the port. Note that when flashing the firmware, you should close any serial communication first, otherwise the upload might fail.

## 3 TESTING

After uploading the code, you can check if the system is functioning correctly by observing the RGB LED colors and the serial output.

!!! tip
    If the RGB LED color is always white, it indicates that there is a problem during the initialization process. Please check the hardware connections. A sky blue state indicates RF communication, meaning it is waiting to communicate with the main node and needs to be tested in conjunction with the main node's signal. Green indicates that initialization is complete and it has entered the IDLE state, waiting for commands from the main node. Yellow is the preparation state before data collection, purple is the state of data collection, and red is the error state, which may be caused by sensor initialization failure or other issues.