#include "timesync.hpp"
#include "time.hpp"
#include <WiFiUdp.h>
#include <NTPClient.h>

// Local NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);  // UTC+8, this will not automatically sync, you need to manually call update()


bool sync_time_ntp()
{
    timeClient.begin();

    if (!timeClient.update())
    {
        Serial.println("[COMMUNICATION] <NTP> Failed to get NTP time.");
        return false;
    }

    uint64_t epoch = (uint64_t)timeClient.getEpochTime();

    // Update global time
    Time.set_time_epoch(epoch);

    Time.last_update_epoch = epoch;
    Time.last_update_ms = millis();
    Time.mcu_time_ms = millis();
    Time.delta_ms = 0;

    Serial.print("[COMMUNICATION] <NTP> Synchronized UNIX epoch: ");
    Serial.println(epoch);

    Serial.print("[COMMUNICATION] <NTP> Current time: ");
    Time.print();

    return true;
}
