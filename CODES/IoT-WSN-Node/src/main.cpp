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

    // Initialize MQTT
    mqtt_setup();
    node_status.node_flags.mqtt_connected = true;
#endif

    node_status.set_state(NodeState::IDLE);
    node_status.print_state();
    rgbled_set_by_state(NodeState::IDLE);

    // === 1. Record sync point ===
    Time.record_sync_time();  // Use global instance
    delay(1234);              // Simulate some elapsed time

    // === 2. Get current unified time ===
    uint64_t unix_ms = Time.get_time();
    Serial.print("Unified Time (ms): ");
    Serial.println(unix_ms);

    // === 3. Convert to calendar ===
    CalendarTime cal = calendar_from_unix_milliseconds(unix_ms);

    // === 4. Print formatted calendar time ===
    Serial.print("Calendar Time     : ");
    Serial.print(cal.year); Serial.print("-");
    if (cal.month < 10) Serial.print("0");
    Serial.print(cal.month); Serial.print("-");
    if (cal.day < 10) Serial.print("0");
    Serial.print(cal.day); Serial.print(" ");
    if (cal.hour < 10) Serial.print("0");
    Serial.print(cal.hour); Serial.print(":");
    if (cal.minute < 10) Serial.print("0");
    Serial.print(cal.minute); Serial.print(":");
    if (cal.second < 10) Serial.print("0");
    Serial.print(cal.second); Serial.print(".");
    if (cal.ms < 100) Serial.print("0");
    if (cal.ms < 10) Serial.print("0");
    Serial.println(cal.ms);

    // === 5. Backward conversion ===
    uint64_t back_sec = unix_from_calendar_seconds(cal);
    uint64_t back_ms = unix_from_calendar_milliseconds(cal);

    Serial.print("Back to Unix Time (s): ");
    Serial.println(back_sec);

    Serial.print("Back to Unix Time (ms): ");
    Serial.println(back_ms);

    // === 6. Show all (encapsulated) ===
    Time.show_time();
}

void loop()
{
}
