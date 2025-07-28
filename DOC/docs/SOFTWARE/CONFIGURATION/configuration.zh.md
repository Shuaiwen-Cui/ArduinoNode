# 配置

为了方便对单片机进行配置，我们专门设置了一个配置文件`config.hpp`,其中集成了大多数可以配置的参数。

!!! tip
    值得注意的是，为了方便采样控制，我们把一些采样相关的全局变量也放在了配置文件中。这样可以方便地在其他文件中引用和修改这些变量。

主节点示例

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

子节点示例

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

如上面代码所示，配置文件中包含了节点信息、WiFi凭据和MQTT配置等。

除去配置信息以外，配置文件还定义了打印信息的函数`print_node_config()`，用于在串口输出当前节点的配置信息，具体定义在`config.cpp`文件中。

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


## 节点模式

对于无线传感器网络而言，有两种概念：网关节点（GATEWAY）和传感器节点（LEAFNODE）。对应的，我们有两个宏来定义节点类型，从而实现代码中的条件编译。

!!! tip 
    在配置文件中，两个宏`GATEWAY`和`LEAFNODE`是互斥的。你只能选择其中一个来定义你的节点类型。

## 节点编号

每个节点都有一个唯一的编号，网关节点的编号为100(与子节点不同即可)，传感器节点的编号从1到NUM_NODES。你可以通过修改`NODE_ID`宏来设置当前节点的编号。

## WiFi凭据

每个节点都需要连接到WiFi网络，因此需要提供WiFi的SSID和密码。你可以在配置文件中修改`WIFI_SSID`和`WIFI_PASSWORD`宏来设置你的WiFi凭据。

!!! warning
    由于Arduino功能限制，目前并不支持连接到校园网络等企业级WiFi网络。请使用家庭或个人WiFi网络。建议使用手机热点，在配置文件中设置`WIFI_SSID`和`WIFI_PASSWORD`为手机热点的SSID和密码。

## MQTT配置

每个节点都需要连接到MQTT服务器，因此需要提供MQTT的相关配置。你可以在配置文件中修改以下宏：

- `MQTT_CLIENT_ID`: 设置当前节点的MQTT客户端ID。网关节点为`GATEWAY`，传感器节点为`LEAFNODE1`、`LEAFNODE2`等。
- `MQTT_BROKER_ADDRESS`: 设置MQTT服务器的地址。使用代码中默认的地址即可。
- `MQTT_BROKER_PORT`: 设置MQTT服务器的端口。使用代码中默认的端口即可。
- `MQTT_USERNAME`: 设置MQTT服务器的用户名。使用代码中默认的用户名
- `MQTT_PASSWORD`: 设置MQTT服务器的密码。使用代码中默认的密码。
- `MQTT_TOPIC_PUB`: 设置MQTT服务器的发布主题。使用代码中默认的主题即可。
- `MQTT_TOPIC_SUB`: 设置MQTT服务器的订阅主题。使用代码中默认的主题即可。

!!! warning
    请确保节点模式，节点编号，和MQTT客户端ID一致，此外，请确保其他信息正确无误，否则可能会导致节点无法正常工作。