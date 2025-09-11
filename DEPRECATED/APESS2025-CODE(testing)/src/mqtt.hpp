#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>
#include "mqtt_cmd.hpp"


// Create a WiFi client and wrap it in PubSubClient

extern WiFiClient wifi_client;
extern PubSubClient mqtt_client;

// === Retrieval Filename
extern char retrieval_filename[32];

// to slow down MQTT loop
bool should_run_mqtt_loop();

// Initialize and connect to MQTT broker
void mqtt_setup();

// Call this in loop() to keep MQTT alive
void mqtt_loop();

// Publish a test message to broker
void mqtt_publish_test();
