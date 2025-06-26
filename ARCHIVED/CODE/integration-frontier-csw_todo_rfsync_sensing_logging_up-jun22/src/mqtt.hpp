#pragma once
#include <Arduino.h>

// Initialize and connect to MQTT broker
void mqtt_setup();

// Call this in loop() to keep MQTT alive
void mqtt_loop();

// Publish a test message to broker
void mqtt_publish_test();
