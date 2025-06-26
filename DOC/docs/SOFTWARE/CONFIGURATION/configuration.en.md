# CONFIGURATION

To facilitate the configuration of the software, we provide a configuration file that can be easily modified. This file is located in the `config` directory of the software installation.

GATEWAY NODE EXAMPLE

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

LEAFNODE EXAMPLE

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

As shown in the code snippet above, the configuration file includes node information, WiFi credentials, and MQTT configurations.

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