#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include "rgbled.hpp"
#include "wifi.hpp"
#include "mpu6050.hpp"
#include "sdcard.hpp"
#include "mqtt.hpp"
#include "time.hpp"
#include "timesync.hpp"
#include "rf.hpp"
#include "sensing.hpp"
#include "logging.hpp"

uint64_t now_unix_ms = 0; // Current Unix time in milliseconds

void setup()
{
  // Entering BOOT state
  node_status.set_state(NodeState::BOOT);

  // Initialize serial communication
  delay(3000); // Wait for serial to initialize
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
  if (sdcard_init(10)) // CS pin = 10
  {
    node_status.node_flags.sd_ready = true;
  }

  if (!rf_init())
  {
    while (1)
      ;
  }
  node_status.node_flags.rf_ready = true;

  // Setup complete: switch to COMMUNICATING state
  node_status.set_state(NodeState::COMMUNICATING);
  rgbled_set_all(CRGB::Blue);

  // Initialize WiFi
  connect_to_wifi();
  node_status.node_flags.wifi_connected = true;

  // Synchronize time via NTP
  while (!sync_time_ntp())
  {
    Serial.println("[COMMUNICATION] <NTP> time sync failed. Retrying in 2 seconds...");
    delay(2000);
  }
  Serial.println("[COMMUNICATION] <NTP> time sync successful.");
  node_status.node_flags.time_ntp_synced = true;

  // Initialize MQTT
  mqtt_setup();
  node_status.node_flags.mqtt_connected = true;

  // RF-based online check & RTT estimation
  rgbled_set_all(CRGB::Yellow);
  sync_check_rf_online();

  // RF time synchronization
  if (!rf_time_sync())
  {
    Serial.println("[COMMUNICATION] <SYNC> RF time sync failed.");
    node_status.set_state(NodeState::ERROR);
    rgbled_set_all(CRGB::Red);
    node_status.print_state();
    while (1)
      ;
  }
  if (node_status.node_flags.time_rf_synced == false)
  {
    Serial.println("[COMMUNICATION] <SYNC> RF time sync failed.");
    node_status.set_state(NodeState::ERROR);
    rgbled_set_all(CRGB::Red);
    node_status.print_state();
    while (1)
      ;
  }

  // Setup complete: switch to IDLE state
  node_status.set_state(NodeState::IDLE);
  rgbled_set_all(CRGB::Green); // green is to show ready
  node_status.print_state();
  delay(3000);
  rgbled_set_all(CRGB::White); // Set LED to white when idle

  // Schedule sensing start time
  SensingSchedule.year = 2025;
  SensingSchedule.month = 6;
  SensingSchedule.day = 23;
  SensingSchedule.hour = 22;
  SensingSchedule.minute = 00;
  SensingSchedule.second = 0;
  SensingSchedule.ms = 0;

  sensing_scheduled_start_ms = SensingSchedule.compute_ms_from_calendar();
}

void loop()
{
  if (node_status.get_state() == NodeState::SAMPLING) // if SAMPLING state, do sampling
  {
    // sampling operation

    // check whether to exit SAMPLING state
    now_unix_ms = Time.estimate_time_ms();
    if (now_unix_ms > sensing_scheduled_end_ms)
    {
      // Switch to IDLE state
      node_status.set_state(NodeState::IDLE);
      rgbled_set_all(CRGB::White); // Set LED to white when idle
    }
  }
  else
  {
    // routine operation
    if (should_run_mqtt_loop())
    {
      mqtt_loop();
      mqtt_publish_test();
    }

    // mqtt_publish_test();

    // check whether to enter SAMPLING state
    now_unix_ms = Time.estimate_time_ms();
    if (sensing_scheduled_start_ms > 0 &&
        sensing_scheduled_end_ms > sensing_scheduled_start_ms &&
        now_unix_ms >= sensing_scheduled_start_ms &&
        now_unix_ms <= sensing_scheduled_end_ms)
    {

      // Switch to SAMPLING state
      node_status.set_state(NodeState::SAMPLING);
      rgbled_set_all(CRGB::Purple); // Set LED to purple during sampling
    }
  }
}
