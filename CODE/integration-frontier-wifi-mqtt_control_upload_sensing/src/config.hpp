#pragma once

/* Node Information */
#define GATEWAY // either GATEWAY or LEAFNODE, only one should be defined
// #define LEAFNODE
#define NODE_ID 100

/* WiFi Credentials */
#define WIFI_SSID "CSW@CEE"
#define WIFI_PASSWORD "88888888"

/* MQTT Configurations */
#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "ArduinoNode_Development"
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"