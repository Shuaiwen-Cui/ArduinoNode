#pragma once

#include "config.hpp"

// === MQTT Flag ===
extern bool mqtt_enabled; // when this is disabled, MQTT will not send heartbeat

// === MQTT Control Flags ===
extern bool flag_command1;
extern bool flag_command2;

// === Function Declarations ===
void mqtt_setup();
void mqtt_publish_test();
void mqtt_loop();
