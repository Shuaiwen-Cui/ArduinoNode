#pragma once

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
// #define NODE_ID 1 // for LEAFNODE: 1, 2, 3, 4
// #define NODE_ID 2
// #define NODE_ID 3
#define NODE_ID 4

#define NUM_NODES 4 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "CSW@CEE"
#define WIFI_PASSWORD "88888888"

// #define WIFI_SSID "Shaun's Iphone"
// #define WIFI_PASSWORD "cshw0918"

// #define WIFI_SSID "7's iPhone"
// #define WIFI_PASSWORD "zhangqi123"

/* MQTT Configurations */
// #define MQTT_CLIENT_ID      "GATEWAY"
// #define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"
// #define MQTT_CLIENT_ID      "LEAFNODE3"
#define MQTT_CLIENT_ID      "LEAFNODE4"

#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"