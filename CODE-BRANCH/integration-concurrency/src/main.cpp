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

  // Initiate RF communication
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
}

void loop()
{
  if (node_status.get_state() == NodeState::IDLE)
  {
    // routine operation
    if (should_run_mqtt_loop())
    {
      mqtt_loop(); // this is important to keep MQTT alive
      mqtt_publish_test();
    }

    // check state change
    now_unix_ms = Time.estimate_time_ms();
    if (node_status.node_flags.sensing_scheduled && now_unix_ms >= sensing_scheduled_start_ms - SENSING_PREPARING_DUR_MS)
    {
      // Switch to PREPARING state
      node_status.set_state(NodeState::PREPARING);
      rgbled_set_all(CRGB::Orange); // Set LED to orange during preparing
    }
  }
  else if (node_status.get_state() == NodeState::COMMUNICATING)
  {
    // check whether to do NTP sync
    if (node_status.node_flags.gateway_ntp_required || node_status.node_flags.leafnode_ntp_required)
    {
      if (!wifi_client.connected())
      {
        Serial.println("[COMMUNICATION] <NTP> Gateway/Leafnode NTP sync required, but WiFi not connected. Reconnecting...");
        connect_to_wifi();
        node_status.node_flags.wifi_connected = true;
      }
      while (!sync_time_ntp())
      {
        Serial.println("[COMMUNICATION] <NTP> time sync failed. Retrying in 2 seconds...");
        delay(2000);
      }
      node_status.node_flags.gateway_ntp_required = false;
      node_status.node_flags.leafnode_ntp_required = false;
    }

    // check whether need to upload data
    if (node_status.node_flags.data_retrieval_requested)
    {
      Serial.print("[COMMUNICATION] <RETRIEVAL> Data retrieval requested. Filename: ");
      Serial.println(retrieval_filename);

      sensing_retrieve_file();
    }

    // check state change
    if (node_status.node_flags.data_retrieval_requested == false && node_status.node_flags.gateway_ntp_required == false && node_status.node_flags.leafnode_ntp_required == false)
    {
      // Switch to IDLE state after data retrieval
      node_status.set_state(NodeState::IDLE);
      rgbled_set_all(CRGB::White); // Set LED to white when idle
      Serial.println("[COMMUNICATION] Data retrieval completed, switching to IDLE state.");
    }
  }
  else if (node_status.get_state() == NodeState::PREPARING)
  {
    // preparing operation

    // check state change
    now_unix_ms = Time.estimate_time_ms();
    if (now_unix_ms >= sensing_scheduled_start_ms)
    {
      // Switch to SAMPLING state
      node_status.set_state(NodeState::SAMPLING);
      rgbled_set_all(CRGB::Purple); // Set LED to purple during sampling
    }
  }
  else if (node_status.get_state() == NodeState::SAMPLING)
  {
    if (!node_status.node_flags.sensing_active)
    {
      if (sensing_start())
      {
        node_status.node_flags.sensing_active = true;
      }
      else
      {
        Serial.println("[ERROR] Sensing start failed.");
        node_status.set_state(NodeState::ERROR);
      }
    }

    sensing_sample_once();

    now_unix_ms = Time.estimate_time_ms();
    if (now_unix_ms > sensing_scheduled_end_ms)
    {
      sensing_stop();
      node_status.node_flags.sensing_active = false;
      node_status.node_flags.sensing_requested = false;
      node_status.node_flags.sensing_scheduled = false;

      node_status.set_state(NodeState::IDLE);
      rgbled_set_all(CRGB::White);
      Serial.println("[STATUS] Sampling completed, switching to IDLE state.");
    }
  }
  else
  {
    // error state
    Serial.println("[ERROR] Node is in an error state. Please check the system.");
    rgbled_set_all(CRGB::Red); // Set LED to red in error state
    delay(1000);               // Blink red LED every second
  }
}
