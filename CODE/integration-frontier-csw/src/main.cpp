#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include "rgbled.hpp"
#include "wifi.hpp"
#include "mpu6050.hpp"
#include "sdcard.hpp"
#include "mqtt.hpp"

void setup()
{
  // Entering BOOT state
  node_status.set_state(NodeState::BOOT);

  // Initialize serial communication
  delay(3000);  // Wait for serial to initialize
  Serial.begin(115200);
  while (!Serial)
    ;
  node_status.node_flags.serial_ready = true;
  Serial.println("[INIT] <Serial> Initialized at 115200 baud.");

  // Initialize RGB LED and show BOOT color (e.g., orange)
  rgbled_init();
  node_status.node_flags.led_ready = true;
  rgbled_set_all(CRGB::Orange);

  // Initialize IMU
  imu_init();
  node_status.node_flags.imu_ready = true;

  // Initialize SD card
  if (sdcard_init(10))  // CS pin = 10
  {
    node_status.node_flags.sd_ready = true;
  }

  // Setup complete: switch to COMMUNICATING state
  node_status.set_state(NodeState::COMMUNICATING);
  rgbled_set_all(CRGB::Blue);

  // Initialize WiFi
  connect_to_wifi();
  node_status.node_flags.wifi_connected = true;

  // Initialize MQTT
  mqtt_setup();
  node_status.node_flags.mqtt_connected = true;

  // Setup complete: switch to IDLE state
  node_status.set_state(NodeState::IDLE);
  rgbled_set_all(CRGB::Green);
  node_status.print_state();
  delay(2000);
}

// void loop()
// {
//   int16_t ax, ay, az;
//   imu_get_acceleration(ax, ay, az);

//   Serial.print("AX: "); Serial.print(ax);
//   Serial.print(" AY: "); Serial.print(ay);
//   Serial.print(" AZ: "); Serial.println(az);

//   delay(500);
// }

void loop()
{
  mqtt_loop();

  mqtt_publish_test();
  delay(5000);
}