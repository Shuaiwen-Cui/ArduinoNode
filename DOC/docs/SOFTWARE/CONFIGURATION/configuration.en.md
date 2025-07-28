# CONFIGURATION

To facilitate the configuration of the software, we provide a configuration file that can be easily modified. This file is located in the `config` directory of the software installation.

!!! tip
    Note that we also place some global variables related to sensing control in the configuration file for easy reference and modification in other files.

GATEWAY NODE EXAMPLE

```cpp
#pragma once
#include <Arduino.h>

/* Node Information */
#define GATEWAY          // for main node
// #define LEAFNODE        // for sensor node

#define NODE_ID 100      // GATEWAY should be 100
// #define NODE_ID 1 // for LEAFNODE: 1, 2
// #define NODE_ID 2

#define NUM_NODES 2 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "Shaun's Iphone"
#define WIFI_PASSWORD "cshw0918"

/* MQTT Configurations */
#define MQTT_CLIENT_ID      "GATEWAY"
// #define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"

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

LEAFNODE EXAMPLE

```cpp
#pragma once
#include <Arduino.h>

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
#define NODE_ID 1 // for LEAFNODE: 1, 2
// #define NODE_ID 2

#define NUM_NODES 2 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "Shaun's Iphone"
#define WIFI_PASSWORD "cshw0918"

/* MQTT Configurations */
// #define MQTT_CLIENT_ID      "GATEWAY"
#define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"

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

As shown in the code snippet above, the configuration file includes node information, WiFi credentials, and MQTT configurations.

Apart from the configuration file, there is also a function `print_node_config()` that can be used to print the current node's configuration information to the serial console. It is defined in the `config.cpp` file, and you can call it in the `setup()` function to display the current node's configuration.

```cpp
#include "config.hpp"

uint64_t sensing_scheduled_start_ms = 0;
uint64_t sensing_scheduled_end_ms = 0;
uint32_t default_sensing_rate_hz = 200;
uint32_t default_sensing_duration_s = 300;
uint32_t sensing_rate_hz = default_sensing_rate_hz ;
uint32_t sensing_duration_s = default_sensing_duration_s;
uint16_t parsed_freq = 0;
uint16_t parsed_duration = 0;

float cali_scale_x = 1.0f; // Calibration scale for X-axis
float cali_scale_y = 1.0f; // Calibration scale for Y-axis
float cali_scale_z = 1.0f; // Calibration scale for Z-axis

void print_node_config()
{
  Serial.println("=== Node Configuration Info ===");

#ifdef GATEWAY
  Serial.println("Node Type     : GATEWAY");
#endif
#ifdef LEAFNODE
  Serial.println("Node Type     : LEAFNODE");
#endif

  Serial.print("Node ID       : ");
  Serial.println(NODE_ID);

  Serial.print("Total Nodes   : ");
  Serial.println(NUM_NODES);

  Serial.println("------ WiFi ------");
  Serial.print("SSID          : ");
  Serial.println(WIFI_SSID);
  Serial.print("Password      : ");
  Serial.println(WIFI_PASSWORD);

  Serial.println("------ MQTT ------");
  Serial.print("Client ID     : ");
  Serial.println(MQTT_CLIENT_ID);
  Serial.print("Broker Addr   : ");
  Serial.println(MQTT_BROKER_ADDRESS);
  Serial.print("Broker Port   : ");
  Serial.println(MQTT_BROKER_PORT);
  Serial.print("Username      : ");
  Serial.println(MQTT_USERNAME);
  Serial.print("Password      : ");
  Serial.println(MQTT_PASSWORD);
  Serial.print("Pub Topic     : ");
  Serial.println(MQTT_TOPIC_PUB);
  Serial.print("Sub Topic     : ");
  Serial.println(MQTT_TOPIC_SUB);

  Serial.println("===============================");
}

```

## Node Modes

For wireless sensor networks, there are two concepts: gateway node (GATEWAY) and sensor node (LEAFNODE). Correspondingly, we have two macros to define the node type, enabling conditional compilation in the code.

!!! tip 
    In the configuration file, the two macros `GATEWAY` and `LEAFNODE` are mutually exclusive. You can only choose one to define your node type.

## Node ID

Each node has a unique ID, with the gateway node having an ID of 100 (different from the sensor nodes), and the sensor nodes numbered from 1 to `NUM_NODES`. You can set the current node's ID by modifying the `NODE_ID` macro.

## WiFi Credentials

Each node needs to connect to a WiFi network, so you need to provide the WiFi SSID and password. You can set your WiFi credentials by modifying the `WIFI_SSID` and `WIFI_PASSWORD` macros in the configuration file.

!!! warning
    Due to the limitations of Arduino, it currently does not support connecting to enterprise-level WiFi networks such as campus networks. Please use a home or personal WiFi network. It is recommended to use a mobile hotspot and set `WIFI_SSID` and `WIFI_PASSWORD` in the configuration file to the SSID and password of your mobile hotspot.

## MQTT Configurations

Each node needs to connect to an MQTT server, so you need to provide the relevant MQTT configurations. You can modify the following macros in the configuration file:

- `MQTT_CLIENT_ID`: Set the MQTT client ID for the current node. The gateway node is set to `GATEWAY`, and sensor nodes are set to `LEAFNODE1`, `LEAFNODE2`, etc.
- `MQTT_BROKER_ADDRESS`: Set the address of the MQTT broker. The default address in the code can be used.
- `MQTT_BROKER_PORT`: Set the port of the MQTT broker, typically 1883. This is the default port for MQTT.
- `MQTT_USERNAME`: Set the username for MQTT authentication. The default username in the code can be used.
- `MQTT_PASSWORD`: Set the password for MQTT authentication. The default password in the code can be used.
- `MQTT_TOPIC_PUB`: Set the topic for publishing messages. The default topic in the code can be used.
- `MQTT_TOPIC_SUB`: Set the topic for subscribing to messages. The default topic in the code can be used.

!!! warning
    Please ensure that the node mode, node ID, and MQTT client ID are consistent. Besides, please ensure all other information is correct, as incorrect configurations may lead to connection failures or unexpected behaviors.