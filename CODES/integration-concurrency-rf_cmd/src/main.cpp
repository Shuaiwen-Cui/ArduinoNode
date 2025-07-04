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
#include "rf_cmd.hpp"
#include "sensing.hpp"
#include "logging.hpp"

uint64_t now_unix_ms = 0;
unsigned long loop_start_ms = 0;

#ifdef GATEWAY
const uint8_t gateway_id = NODE_ID;
const size_t leafnode_count = NUM_NODES - 1;

// Map index to leafnode ID (skipping gateway ID)
uint8_t get_leafnode_id(size_t index)
{
    return (index < gateway_id - 1) ? index + 1 : index + 2;
}
#endif

void setup()
{
    node_status.set_state(NodeState::BOOT);

    delay(3000);
    Serial.begin(115200);
    while (!Serial);
    node_status.node_flags.serial_ready = true;
    Serial.println("[INIT] <Serial> Initialized at 115200 baud.");

    rgbled_init();
    node_status.node_flags.led_ready = true;
    rgbled_set_all(CRGB::Orange);

    imu_init();
    node_status.node_flags.imu_ready = true;

    if (sdcard_init(10))
        node_status.node_flags.sd_ready = true;

#ifdef GATEWAY
    delay(5000);
#endif

    if (!rf_init())
        while (1);
    node_status.node_flags.rf_ready = true;

#ifdef GATEWAY
    rgbled_set_all(CRGB::Blue);
    connect_to_wifi();
    node_status.node_flags.wifi_connected = true;

    while (!sync_time_ntp())
    {
        Serial.println("[COMMUNICATION] <NTP> time sync failed. Retrying in 2 seconds...");
        delay(2000);
    }
    Serial.println("[COMMUNICATION] <NTP> time sync successful.");
    node_status.node_flags.time_ntp_synced = true;

    mqtt_setup();
    node_status.node_flags.mqtt_connected = true;
#endif

    rgbled_set_all(CRGB::Yellow);
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
    rgbled_set_all(CRGB::Green);
    node_status.print_state();
    delay(3000);
    rgbled_set_all(CRGB::White);

    loop_start_ms = millis();
}

void loop()
{
#ifdef GATEWAY
    static bool reboot_sent = false;
    if (!reboot_sent && millis() - loop_start_ms >= 20000)
    {
        Serial.println("[GATEWAY] Sending CMD_REBOOT to all leaf nodes (reliable mode)...");

        bool all_acknowledged = true;
        for (size_t i = 0; i < leafnode_count; ++i)
        {
            uint8_t target_id = get_leafnode_id(i);
            Serial.print("[GATEWAY] Sending CMD_REBOOT to node ");
            Serial.println(target_id);

            bool success = rf_send_command_reliable(target_id, "CMD_REBOOT");
            if (!success)
            {
                Serial.print("[GATEWAY] Node ");
                Serial.print(target_id);
                Serial.println(" did not acknowledge reboot command.");
                all_acknowledged = false;
            }

            delay(200); // spacing to reduce RF congestion
        }

        if (all_acknowledged)
        {
            Serial.println("[GATEWAY] All nodes acknowledged. Rebooting self...");
        }
        else
        {
            Serial.println("[GATEWAY] Some nodes did not respond. Rebooting self anyway.");
        }

        reboot_sent = true;
        rgbled_set_all(CRGB::Red);
        delay(2000);
        NVIC_SystemReset();
    }
#endif

    if (node_status.get_state() == NodeState::IDLE)
    {
#ifdef LEAFNODE
        if (should_run_rf_loop())
            rf_loop_handle_command();

        if (node_status.node_flags.reboot_required_leafnode)
        {
            Serial.println("[LEAFNODE] Reboot command received. Restarting in 2 seconds...");
            rgbled_set_all(CRGB::Red);
            delay(2000);
            NVIC_SystemReset();
        }
#endif

#ifdef GATEWAY
        if (should_run_mqtt_loop())
            mqtt_loop();
#endif

        now_unix_ms = Time.estimate_time_ms();
        if (node_status.node_flags.sensing_scheduled &&
            now_unix_ms >= sensing_scheduled_start_ms - SENSING_PREPARING_DUR_MS)
        {
            node_status.set_state(NodeState::PREPARING);
            rgbled_set_all(CRGB::Orange);
            Serial.println("[STATUS] Switching to PREPARING state.");
        }
    }
    else if (node_status.get_state() == NodeState::COMMUNICATING)
    {
        if (node_status.node_flags.gateway_ntp_required || node_status.node_flags.leafnode_ntp_required)
        {
#ifdef GATEWAY
            if (!wifi_client.connected())
            {
                Serial.println("[COMMUNICATION] <NTP> WiFi not connected. Reconnecting...");
                connect_to_wifi();
                node_status.node_flags.wifi_connected = true;
            }

            while (!sync_time_ntp())
            {
                Serial.println("[COMMUNICATION] <NTP> time sync failed. Retrying in 2 seconds...");
                delay(2000);
            }
#endif
            node_status.node_flags.gateway_ntp_required = false;
            node_status.node_flags.leafnode_ntp_required = false;
        }

        if (node_status.node_flags.data_retrieval_requested)
        {
            Serial.print("[COMMUNICATION] <RETRIEVAL> Data retrieval requested. Filename: ");
            Serial.println(retrieval_filename);
            sensing_retrieve_file();
        }

        if (!node_status.node_flags.gateway_ntp_required &&
            !node_status.node_flags.leafnode_ntp_required &&
            !node_status.node_flags.data_retrieval_requested)
        {
            node_status.set_state(NodeState::IDLE);
            rgbled_set_all(CRGB::White);
            Serial.println("[COMMUNICATION] Done. Switching to IDLE state.");
        }
    }
    else if (node_status.get_state() == NodeState::PREPARING)
    {
        now_unix_ms = Time.estimate_time_ms();
        if (now_unix_ms >= sensing_scheduled_start_ms)
        {
            node_status.set_state(NodeState::SAMPLING);
            rgbled_set_all(CRGB::Purple);
        }
    }
    else if (node_status.get_state() == NodeState::SAMPLING)
    {
        if (!node_status.node_flags.sensing_active)
        {
            if (sensing_start())
                node_status.node_flags.sensing_active = true;
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
        Serial.println("[ERROR] Node is in an error state. Please check the system.");
        rgbled_set_all(CRGB::Red);
        delay(1000);
        NVIC_SystemReset();
    }
}
