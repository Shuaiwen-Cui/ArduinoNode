# 配置

为了方便对单片机进行配置，我们专门设置了一个配置文件`config.hpp`,其中集成了大多数可以配置的参数。

主节点示例

```cpp
#pragma once

/* Node Information */
#define GATEWAY          // for main node
// #define LEAFNODE        // for sensor node

#define NODE_ID 100      // GATEWAY should be 100
// for LEAFNODE: 1, 2, 3, 4
// #define NODE_ID 1
// #define NODE_ID 2
// #define NODE_ID 3
// #define NODE_ID 4

#define NUM_NODES 4 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "CSW@CEE"
#define WIFI_PASSWORD "88888888"

/* MQTT Configurations */
#define MQTT_CLIENT_ID      "GATEWAY"
// #define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"
// #define MQTT_CLIENT_ID      "LEAFNODE3"
// #define MQTT_CLIENT_ID      "LEAFNODE4"

#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"

```

子节点示例

```cpp
#pragma once

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
// for LEAFNODE: 1, 2, 3, 4
#define NODE_ID 1
// #define NODE_ID 2
// #define NODE_ID 3
// #define NODE_ID 4

#define NUM_NODES 4 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "CSW@CEE"
#define WIFI_PASSWORD "88888888"

/* MQTT Configurations */
// #define MQTT_CLIENT_ID      "GATEWAY"
#define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"
// #define MQTT_CLIENT_ID      "LEAFNODE3"
// #define MQTT_CLIENT_ID      "LEAFNODE4"

#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"

```

如上面代码所示，配置文件中包含了节点信息、WiFi凭据和MQTT配置等。

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