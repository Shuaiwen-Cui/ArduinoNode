#include <Arduino.h>
#include "wifi.hpp"
#include "mqtt.hpp"

void setup() {
  Serial.begin(115200);
  while (!Serial);

  connect_to_wifi();   // Connect to WiFi
  mqtt_setup();        // Connect to MQTT
}

void loop() {
  mqtt_loop();         // Keep MQTT connection alive
  mqtt_publish_test(); // Send message
  delay(5000);
}
