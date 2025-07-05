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
#include <Arduino.h>     // Arduino Framework
#include "config.hpp"    // Project Configuration
#include "nodestate.hpp" // Node State and Flags Management
#include "rgbled.hpp"    // RGB LED Control Functions
#include "mpu6050.hpp"   // MPU6050 IMU Sensor Functions
#include "sdcard.hpp"    // SD Card Functions
#include "rf.hpp"        // RF Communication Functions
#include "wifi.hpp"      // WiFi Connection Functions
#include "time.hpp"      // Time Synchronization Functions
#include "timesync.hpp"  // Time Synchronization Header
#include "mqtt.hpp"      // MQTT Communication Functions
#include "rf_cmd.hpp"    // RF Command Functions

/*========== HELPERS ==========*/
uint64_t now_unix_ms = 0; // Current Unix time in milliseconds

/*========== SETUP ==========*/
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

    // Switch to RF_COMMUNICATING state
    node_status.set_state(NodeState::RF_COMMUNICATING);
    rgbled_set_by_state(NodeState::RF_COMMUNICATING);

    sync_check_rf_online();

    if (!rf_time_sync() || !node_status.node_flags.time_rf_synced)
    {
        Serial.println("[COMMUNICATION] <SYNC> RF time sync failed.");
        node_status.set_state(NodeState::ERROR);
        rgbled_set_all(CRGB::Red);
        node_status.print_state();
        delay(2000);
        NVIC_SystemReset();
    }

    node_status.set_state(NodeState::IDLE);
    node_status.print_state();
    rgbled_set_by_state(NodeState::IDLE);
}

/*========== LOOP ==========*/

// void loop()
// {
//     now_unix_ms = Time.estimate_time_ms();

// #if defined(GATEWAY)

//     // === GATEWAY: Periodically send RF command to all leaf nodes ===
//     static unsigned long last_send_time = 0;
//     const unsigned long send_interval = 5000;

//     if (millis() - last_send_time >= send_interval)
//     {
//         last_send_time = millis();

//         // Construct RF command message
//         RFMessage msg;
//         msg.from_id = NODE_ID;

//         static int counter = 0;
//         counter++;

//         // Alternate command for test
//         if (counter % 2 == 0)
//             strncpy(msg.payload, "CMD1", sizeof(msg.payload));
//         else
//             strncpy(msg.payload, "CMD2", sizeof(msg.payload));

//         // Send to all leaf nodes
//         for (uint8_t target_id = 1; target_id <= NUM_NODES; ++target_id)
//         {
//             msg.to_id = target_id;

//             Serial.print("[GATEWAY] Sending command to Node ");
//             Serial.print(target_id);
//             Serial.print(": ");
//             Serial.println(msg.payload);

//             node_status.set_state(NodeState::RF_COMMUNICATING);
//             rgbled_set_by_state(NodeState::RF_COMMUNICATING);

//             rf_stop_listening();
//             rf_send(msg.to_id, msg);
//             rf_start_listening();
//         }

//         node_status.set_state(NodeState::IDLE);
//         rgbled_set_by_state(NodeState::IDLE);
//     }

// #elif defined(LEAFNODE)

//     // === LEAFNODE: Handle incoming RF command ===
//     rf_handle();

// #endif
// }

void loop()
{
    now_unix_ms = Time.estimate_time_ms();

    static NodeState prev_state = NodeState::BOOT;
    NodeState current_state = node_status.get_state();

    static unsigned long idle_start_time = 0;

    // === Update idle_start_time only when entering IDLE state ===
    if (current_state == NodeState::IDLE && prev_state != NodeState::IDLE)
    {
        idle_start_time = millis();
        Serial.println("[SYSTEM] Entered IDLE state.");
    }

    prev_state = current_state; // update for next loop

    if (current_state == NodeState::BOOT)
    {
        delay(3000);        // Wait for 3 seconds in BOOT state
        NVIC_SystemReset(); // Reset the system
    }
    else if (current_state == NodeState::IDLE)
    {
#ifdef GATEWAY
    static bool sensing_cmd_sent = false;
    static unsigned long idle_start_time = 0;

    if (idle_start_time == 0)
        idle_start_time = millis();

    if (!sensing_cmd_sent && millis() - idle_start_time > 10000)
    {
        sensing_cmd_sent = true;

        const char *sensing_cmd = "S_251105123000_100_10"; // YYMMDDHHMMSS + rate + duration
        rf_command(sensing_cmd);
    }
#endif

#ifdef LEAFNODE
        rf_handle();

        if (node_status.node_flags.reboot_required_leafnode)
        {
            Serial.println("[LEAFNODE] Reboot flag detected. Switching to BOOT state...");
            node_status.set_state(NodeState::BOOT);
            rgbled_set_by_state(NodeState::BOOT);
        }

        if (node_status.node_flags.time_rf_required)
        {
            Serial.println("[LEAFNODE] Performing RF sync...");
            sync_check_rf_online();
            rf_time_sync();
            node_status.node_flags.time_rf_required = false;

            node_status.set_state(NodeState::IDLE);
            rgbled_set_by_state(NodeState::IDLE);
        }
#endif
    }
    else if (current_state == NodeState::ERROR)
    {
        rgbled_set_by_state(NodeState::ERROR);
        delay(3000);
        Serial.println("[ERROR] System in error state. Rebooting...");
        NVIC_SystemReset();
    }
}
