#pragma once

// === MQTT CONFIGURATION ===
#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "ArduinoNode_Development"
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"

// === MQTT Flag ===
extern bool mqtt_enabled; // when this is disabled, MQTT will not send heartbeat

// === MQTT Control Flags ===
extern bool flag_command1;
extern bool flag_command2;

// === Function Declarations ===
void mqtt_setup();
void mqtt_publish_test();
void mqtt_loop();
