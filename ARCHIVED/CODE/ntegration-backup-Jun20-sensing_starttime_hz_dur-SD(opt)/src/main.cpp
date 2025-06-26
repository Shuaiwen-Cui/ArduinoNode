#include <Arduino.h>
#include "config.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "time.hpp"
#include "sensing.hpp"
#include "sdcard.hpp"

unsigned long last_mqtt_time = 0;
const unsigned long mqtt_interval = 100;

void setup()
{
    delay(3000);
    Serial.begin(115200);
    while (!Serial);

    connect_to_wifi();
    time_init();

    while (!sync_time_ntp(global_time))
    {
        Serial.println("[Setup] NTP synchronization failed. Retrying...");
        delay(2000);
    }
    Serial.println("[Setup] Time synchronized via NTP.");

    mqtt_setup();
    mpu_init();

    if (!sdcard_init(10))
    {
        Serial.println("[Main] SD card setup failed.");
        while (1);
    }

    // Optional manual sensing
    // NodeTime start_time = {2025, 6, 20, 13, 37, 0, 0, 0};
    // sensing_prepare(start_time, 100, 20);
    Serial.println("[Main] Node initialization finished.");
}

void loop()
{
    unsigned long now = millis();
    if (now - last_mqtt_time >= mqtt_interval)
    {
        last_mqtt_time = now;
        mqtt_loop();
    }

    if (cmd_flg_ntp)
    {
        while (!sync_time_ntp(global_time))
        {
            Serial.println("[NTP] Sync failed. Retrying...");
            delay(2000);
        }
        Serial.println("[NTP] Sync successful.");
        cmd_flg_ntp = false;
        mqtt_enabled = true;
    }

    if (cmd_flg_scheduled_sensing)
    {
        Serial.println("[CMD_SENSING] Processing scheduled sensing command...");
        cmd_flg_scheduled_sensing = false;

        if (sensing_prepare(parsed_start_time, parsed_freq, parsed_duration))
        {
            Serial.println("[CMD_SENSING] Sensing task prepared successfully.");
            parsed_start_time.print();
        }
        else
        {
            Serial.println("[CMD_SENSING] Failed to prepare sensing.");
        }
    }

    sensing_update();

    if (sensing_is_full())
    {
        sensing_set_flag(false);
        sensing_flush();
    }
}
