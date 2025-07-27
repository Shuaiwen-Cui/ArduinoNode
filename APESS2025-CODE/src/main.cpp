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
#include "timesync.hpp"  // Time Synchronization Logic
#include "mqtt.hpp"      // MQTT Communication Functions
#include "sensing.hpp"   // Sensing Functions
#include "rf_cmd.hpp"    // RF Command Handling Functions

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

    // RF communication
    // Switch to RF_COMMUNICATING state
    node_status.set_state(NodeState::RF_COMMUNICATING);
    rgbled_set_by_state(NodeState::RF_COMMUNICATING);
    rf_check_node_status();
    Serial.println("[INIT] <RF> Node status checked.");

    // delay(1000);
    // void rf_sync_log_number();
    // Serial.println("[INIT] <RF> Log number synchronized.");

    if (!rf_time_sync())
    {
        Serial.println("[ERROR] Time sync failed.");
        while (1)
            ;
    }
    Serial.println("[INIT] <RF> Time sync successful.");
    node_status.node_flags.time_rf_synced = true;

    node_status.set_state(NodeState::IDLE);
    node_status.print_state();
    rgbled_set_by_state(NodeState::IDLE);

    // simulate sensing triggering
    // delay(3000);
    // node_status.node_flags.sensing_scheduled = true;
    // sensing_scheduled_start_ms = Time.get_time() + 10000;
    // sensing_rate_hz = 200;
    // sensing_duration_s = 10;
    // sensing_scheduled_end_ms = sensing_scheduled_start_ms + sensing_duration_s*1000;
    // Serial.println('sensing scheduled');
}

/*========== LOOP ==========*/
void loop()
{

    if (node_status.get_state() == NodeState::BOOT)
    {
        delay(3000);        // Wait for 3 seconds in BOOT state
        NVIC_SystemReset(); // Reset the system
    }
    else if (node_status.get_state() == NodeState::IDLE)
    {

        // === Debug: Show time every second in IDLE state ===
        // static uint64_t last_show_time = 0;     // static: only initialized once, persists across loop calls
        // uint64_t now_unix_ms = Time.get_time(); // Get current time from Time.get_time()

        // if (now_unix_ms - last_show_time >= 1000) // Check if 1 second has passed
        // {
        //     last_show_time = now_unix_ms; // Update the last show time
        //     Serial.print("[DEBUG] Time in IDLE state: ");
        //     Time.show_time(); // Call Time.show_time() to display current time
        // }

        // === Check for Reboot ===
#ifdef GATEWAY
        if (node_status.node_flags.reboot_required_leafnode)
        {
            Serial.println("[GATEWAY] Reboot command received for leafnodes.");
            send_command_with_retry("CMD_REBOOT"); // Send reboot command to all leafnodes
        }
        if (node_status.node_flags.reboot_required_gateway)
        {
            Serial.println("[GATEWAY] Reboot command received for gateway.");
            delay(3000); // Wait for 1 second before rebooting
            node_status.set_state(NodeState::BOOT);
        }
#endif
#ifdef LEAFNODE
        if (node_status.node_flags.reboot_required_leafnode)
        {
            Serial.println("[LEAFNODE] Reboot command received for leaf node.");
            node_status.set_state(NodeState::BOOT);
        }
#endif

#ifdef GATEWAY
        // === Check for RF Sync Request from MQTT ===
        if (node_status.node_flags.time_rf_required)
        {
            Serial.println("[GATEWAY] RF time sync requested via MQTT.");
            send_command_with_retry("CMD_RF_SYNC"); // Send RF sync command
            delay(2000);                            // Wait for 1 second to allow RF sync to complete
            node_status.set_state(NodeState::RF_COMMUNICATING);
            rgbled_set_by_state(NodeState::RF_COMMUNICATING);
        }
#endif

#ifdef GATEWAY
        // === Routine MQTT maintenance ===
        if (should_run_mqtt_loop())
        {
            // Check WiFi connection, reconnect if disconnected
            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println("[MQTT] WiFi disconnected. Reconnecting...");
                connect_to_wifi(); // Reconnect to WiFi
            }

            mqtt_loop(); // Keep MQTT connection alive
            // mqtt_publish_test(); // Optional test message
        }
#endif

#ifdef GATEWAY
        // === Check for sensing request from MQTT ===
        if (node_status.node_flags.sensing_requested)
        {
            node_status.node_flags.sensing_requested = false;
            node_status.node_flags.sensing_scheduled = true;

            // Construct sensing command string: S_YYMMDDHHMMSS_<RATE>_<DUR>
            char command_buf[32];
            CalendarTime SensingSchedule = calendar_from_unix_milliseconds(sensing_scheduled_start_ms); // note that the milliseconds are not included in the command
            snprintf(command_buf, sizeof(command_buf),
                     "S_%02d%02d%02d%02d%02d%02d_%d_%d",
                     SensingSchedule.year % 100, // YY
                     SensingSchedule.month,
                     SensingSchedule.day,
                     SensingSchedule.hour,
                     SensingSchedule.minute,
                     SensingSchedule.second,
                     parsed_freq,
                     parsed_duration);

            Serial.print("[GATEWAY] Sending sensing command via RF: ");
            Serial.println(command_buf);

            rf_command(command_buf);
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
            Serial.println("[LEAFNODE] RF sync requested.");

            node_status.set_state(NodeState::RF_COMMUNICATING);
            rgbled_set_by_state(NodeState::RF_COMMUNICATING);
        }
#endif

        // === Check for sensing schedule ===
        // now_unix_ms = Time.estimate_time_ms();
        // if (node_status.node_flags.sensing_scheduled &&
        //     now_unix_ms >= sensing_scheduled_start_ms - SENSING_PREPARING_DUR_MS)
        // {
        //     node_status.set_state(NodeState::PREPARING);
        //     rgbled_set_by_state(NodeState::PREPARING);
        //     Serial.println("[STATUS] Switching to PREPARING state.");
        // }

        now_unix_ms = Time.get_time();

        if (node_status.node_flags.sensing_scheduled)
        {
            static uint64_t last_debug = 0; // static: only initialized once, persists across loop calls
            uint64_t count_down = 0;

            if (now_unix_ms - last_debug >= 1000)
            {
                last_debug = now_unix_ms;
                count_down = (sensing_scheduled_start_ms - now_unix_ms) / 1000; // Countdown in seconds

                Serial.print("[DEBUG] sensing_scheduled = true | countdown = ");
                Serial.print(count_down);
                Serial.println(" seconds");
            }

            // if (now_unix_ms - last_debug >= 1000)
            // {
            //     last_debug = now_unix_ms;

            //     Serial.print("[DEBUG] sensing_scheduled = true | now_unix_ms = ");
            //     Serial.print(now_unix_ms);
            //     Serial.print(", scheduled_start_ms = ");
            //     Serial.print(sensing_scheduled_start_ms);
            //     Serial.print(", diff = ");
            //     Serial.println(sensing_scheduled_start_ms - now_unix_ms);
            // }

            // if (now_unix_ms % 1000 == 0)
            // {
            //     Serial.print("[DEBUG] sensing_scheduled = true | now_unix_ms = ");
            //     Serial.print(now_unix_ms);
            //     Serial.print(", scheduled_start_ms = ");
            //     Serial.print(sensing_scheduled_start_ms);
            //     Serial.print(", diff = ");
            //     Serial.println(sensing_scheduled_start_ms - now_unix_ms);
            // }

            if (now_unix_ms >= sensing_scheduled_start_ms - SENSING_PREPARING_DUR_MS)
            {
                Serial.println("[DEBUG] Condition met: switching to PREPARING.");
                node_status.set_state(NodeState::PREPARING);
                rgbled_set_by_state(NodeState::PREPARING);
            }
        }
    }
    else if (node_status.get_state() == NodeState::WIFI_COMMUNICATING)
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

        // switch to IDLE state after handling WiFi communication
        node_status.set_state(NodeState::IDLE);
        rgbled_set_by_state(NodeState::IDLE);
    }
    else if (node_status.get_state() == NodeState::RF_COMMUNICATING)
    {
        // check whether to do RF time sync
        if (node_status.node_flags.time_rf_required)
        {
            Serial.println("[COMMUNICATION] <SYNC> RF time sync required.");
            rf_time_sync();
            node_status.node_flags.time_rf_required = false;

            node_status.set_state(NodeState::IDLE);
            rgbled_set_by_state(NodeState::IDLE);
        }
    }
    else if (node_status.get_state() == NodeState::PREPARING)
    {
        // preparing operation
        if (!node_status.node_flags.sensing_active)
        {
            if (sensing_prepare())
            {
                node_status.node_flags.sensing_active = true;
            }
            else
            {
                Serial.println("[ERROR] Sensing start failed.");
                node_status.set_state(NodeState::ERROR);
            }
        }

        // check state change
        now_unix_ms = Time.get_time();
        if (now_unix_ms >= sensing_scheduled_start_ms)
        {
            // Switch to SAMPLING state
            node_status.set_state(NodeState::SAMPLING);
            rgbled_set_by_state(NodeState::SAMPLING);
        }
    }
    else if (node_status.get_state() == NodeState::SAMPLING)
    {

        sensing_sample_once();

        now_unix_ms = Time.get_time();
        if (now_unix_ms > sensing_scheduled_end_ms)
        {
            sensing_stop();
            node_status.node_flags.sensing_active = false;
            node_status.node_flags.sensing_requested = false;
            node_status.node_flags.sensing_scheduled = false;

            node_status.set_state(NodeState::IDLE);
            rgbled_set_by_state(NodeState::IDLE);
            Serial.println("[STATUS] Sampling completed, switching to IDLE state.");
        }
    }
    else if (node_status.get_state() == NodeState::ERROR)
    {
        rgbled_set_by_state(NodeState::ERROR);
        delay(3000);
        Serial.println("[ERROR] System in error state. Rebooting...");
        NVIC_SystemReset();
    }
}
