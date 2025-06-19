#include <Arduino.h>
#include "config.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "time.hpp"
#include "sensing.hpp"

unsigned long last_mqtt_time = 0;
unsigned long last_time_print = 0;

const unsigned long mqtt_interval = 100;     // MQTT loop interval (ms)
const unsigned long print_interval = 1000;   // Time print interval (ms)

void setup()
{
  delay(3000);  // Allow time for serial monitor to open
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

  mpu_init();            // Initialize MPU6050 sensor

  // Prepare sensing: xxx Hz, xxx s
  if (sensing_prepare(100, 25))
  {
    sensing_set_flag(true);  // Start sensing
  }
  else
  {
    Serial.println("[Setup] Sensing preparation failed.");
  }
}


void loop()
{
  unsigned long now = millis();

  // MQTT loop with interval
  if (now - last_mqtt_time >= mqtt_interval)
  {
    last_mqtt_time = now;
    mqtt_loop();  // Internal control via mqtt_enabled
  }

  // Command handlers (if needed)
  if (flag_command1)
  {
    Serial.println("[Task] Executing COMMAND1...");
    flag_command1 = false;
    delay(30000);
    mqtt_enabled = true;
  }

  if (flag_command2)
  {
    Serial.println("[Task] Executing COMMAND2...");
    flag_command2 = false;
  }

  // Update sensing
  sensing_update();

  // Auto flush and stop when full
  if (sensing_is_full())
  {
    sensing_set_flag(false);
    sensing_flush();
  }
}

