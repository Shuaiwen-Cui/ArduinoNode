#pragma once

#include "config.hpp"
#include "time.hpp"
#include <PubSubClient.h>

// === MQTT Control Flags ===
extern bool mqtt_enabled;
extern bool cmd_flg_ntp;
extern bool cmd_flg_scheduled_sensing;
extern bool cmd_flg_retrieval;

// === Parsed Command Parameters ===
extern char cmd_sensing_raw[128];
extern NodeTime parsed_start_time;
extern uint16_t parsed_freq;
extern uint16_t parsed_duration;

// === Retrieval Command Parameters ===
extern char retrieval_filename[32];

// === MQTT Function Declarations ===
void mqtt_setup();
void mqtt_publish_test();
void mqtt_loop();

// === Expose MQTT client for external publish
extern PubSubClient mqtt_client;
