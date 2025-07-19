/**
 * @file main.cpp
 * @author SHUAIWEN CUI (SHUAIWEN001@e.ntu.edu.sg)
 * @brief
 * @version 1.0
 * @date 2025-07-01
 *
 * @copyright Copyright (c) 2025
 *
 */

/*========== DEPENDENCIES ==========*/
#include <Arduino.h> // Arduino Framework
#include "config.hpp"    // Project Configuration
#include "nodestate.hpp" // Node State and Flags Management
#include "rgbled.hpp"    // RGB LED Control Functions
#include "mpu6050.hpp"   // MPU6050 IMU Sensor Functions
#include "sdcard.hpp"    // SD Card Functions
#include "rf.hpp"        // RF Communication Functions
#include "wifi.hpp"      // WiFi Connection Functions
#include "time.hpp"      // Time Synchronization Functions
#include "timesync.hpp"  // Time Synchronization Logic
#include "mqtt.hpp"      // MQTT Communication Functions


/*========== HELPERS ==========*/

void setup()
{
    // Entering the boot state
    node_status.set_state(NodeState::BOOT);

    // Serial Initialization
    delay(3000);
    Serial.begin(115200);
    while (!Serial)
    {
        ; // Wait for Serial to be ready
    }
    node_status.node_flags.serial_ready = true; // Serial is ready
    Serial.println("[INIT] <Serial> Initialized at 115200 baud.");
    print_node_config();

    // RGB LED Initialization
    rgbled_init();
    node_status.node_flags.led_ready = true;

    rgbled_set_by_state(NodeState::BOOT);
    delay(2000); // Allow time for the LED to show the boot color

        // Accelerometer Initialization - MPU6050
    imu_init();
    node_status.node_flags.imu_ready = true;

    // SD Card Initialization
    while (!sdcard_init(10))
    {
        Serial.println("[INIT] <SD Card> Initialization failed, retrying...");
        delay(1000); // Retry every second
    }
    node_status.node_flags.sd_ready = true;

    // RF Communication Initialization
    if (!rf_init())
        while (1)
            ;
    node_status.node_flags.rf_ready = true;

#ifdef GATEWAY
    // Switch to WIFI_COMMUNICATING state
    node_status.set_state(NodeState::WIFI_COMMUNICATING);
    rgbled_set_by_state(NodeState::WIFI_COMMUNICATING);

    // Initialize WiFi
    connect_to_wifi();
    node_status.node_flags.wifi_connected = true;

    // Synchronize time via NTP
    while (!sync_time_ntp())
    {
        Serial.println("[INIT] <NTP> time sync failed. Retrying in 2 seconds...");
        delay(2000);
    }
    Serial.println("[INIT] <NTP> time sync successful.");
    node_status.node_flags.time_ntp_synced = true;

    // Initialize MQTT
    mqtt_setup();
    node_status.node_flags.mqtt_connected = true;
#endif

    // RF communication
    // Switch to RF_COMMUNICATING state
    node_status.set_state(NodeState::RF_COMMUNICATING);
    rgbled_set_by_state(NodeState::RF_COMMUNICATING);
    rf_check_node_status();  
    Serial.println("[INIT] <RF> Node status checked.");

    if (!rf_time_sync())
    {
        Serial.println("[ERROR] Time sync failed.");
        while (1);
    }
    Serial.println("[INIT] <RF> Time sync successful.");
    

    node_status.set_state(NodeState::IDLE);
    node_status.print_state();
    rgbled_set_by_state(NodeState::IDLE);

}

void loop()
{
    static uint32_t last_print_time = 0;
    uint32_t now = millis();

    // Print once every 1000 ms
    if (now - last_print_time >= 5000)
    {
        last_print_time = now;
        Time.show_time();  // Use your defined global instance
    }
}
