#pragma once
#include <Arduino.h>

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
#define NODE_ID 1 // for LEAFNODE: 1, 2, 3, 4, 5, 6, 7, 8
// #define NODE_ID 2
// #define NODE_ID 3
// #define NODE_ID 4
// #define NODE_ID 5
// #define NODE_ID 6
// #define NODE_ID 7
// #define NODE_ID 8

#define NUM_NODES 8 // Total number of nodes in the network

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
// #define MQTT_CLIENT_ID      "LEAFNODE7"
// #define MQTT_CLIENT_ID      "LEAFNODE8"

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
