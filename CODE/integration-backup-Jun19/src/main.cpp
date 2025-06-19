#include <Arduino.h>
#include "wifi.hpp"
#include "mqtt.hpp"
#include "time.hpp"

unsigned long last_mqtt_time = 0;
unsigned long last_time_print = 0;

const unsigned long mqtt_interval = 100;     // MQTT loop interval (ms)
const unsigned long print_interval = 1000;   // Time print interval (ms)

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  connect_to_wifi();     // Connect to WiFi
  time_init();           // Initialize time

  if (sync_time_ntp(global_time))
  {
    Serial.println("[Setup] Time synchronized via NTP.");
  }
  else
  {
    Serial.println("[Setup] NTP synchronization failed.");
  }

  mqtt_setup();          // Initialize MQTT
}

void loop()
{
  unsigned long now = millis();

  // MQTT handling
  if (now - last_mqtt_time >= mqtt_interval)
  {
    last_mqtt_time = now;
    mqtt_loop(); // built-in logic: will be halted when conducting tasks, and will resume after tasks are done
  }

  // Print current time every second
  if (now - last_time_print >= print_interval)
  {
    last_time_print = now;

    NodeTime current = get_current_time();
    current.print();  // print to Serial
  }

  // Command handlers
  if (flag_command1)
  {
    Serial.println("[Task] Executing COMMAND1...");
    flag_command1 = false;
    delay(30000);  // simulate some work
    mqtt_enabled = true;  // re-enable MQTT after COMMAND1
  }

  if (flag_command2)
  {
    Serial.println("[Task] Executing COMMAND2...");
    flag_command2 = false;
  }
}
