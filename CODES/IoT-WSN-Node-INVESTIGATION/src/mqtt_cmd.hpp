#pragma once
#include <Arduino.h>
#include "mqtt.hpp"

// Callback when subscribed message is received
void mqtt_callback(char *topic, byte *payload, unsigned int length);