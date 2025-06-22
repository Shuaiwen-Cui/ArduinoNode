
#include "time.hpp"
#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// === Global time instance ===
NodeTime global_time;
unsigned long base_millis_at_sync = 0; // Store millis() when time was last synced

// Create UDP instance and NTP client
static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // +8 timezone, update every 60s

// Print the current NodeTime information in readable format
void NodeTime::print() const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "time: %04d-%02d-%02d %02d:%02d:%02d.%03d, offset: %+d ms",
             year, month, day, hour, minute, second, millisecond, offset_ms);
    Serial.println(buffer);
}

// Initialize the time system and reset global time
void time_init()
{
    global_time = {
        .year = 1970,
        .month = 1,
        .day = 1,
        .hour = 0,
        .minute = 0,
        .second = 0,
        .millisecond = 0,
        .offset_ms = 0
    };
    base_millis_at_sync = millis(); // the local time (ms) since the system started/reset
}

// Synchronize time from NTP server
bool sync_time_ntp(NodeTime &current_time)
{
    timeClient.begin();
    if (!timeClient.forceUpdate())
    {
        Serial.println("[NTP] Time sync failed.");
        return false;
    }

    unsigned long epoch = timeClient.getEpochTime();     // seconds since 1970
    unsigned long ms = millis();                         // for estimating milliseconds
    unsigned long ms_offset = ms % 1000;

    // === Step 1: Extract time ===
    current_time.second = epoch % 60;
    epoch /= 60;
    current_time.minute = epoch % 60;
    epoch /= 60;
    current_time.hour = epoch % 24;
    epoch /= 24;

    // === Step 2: Calculate date ===
    const uint16_t days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    uint16_t days = (uint16_t)epoch;

    uint16_t year = 1970;
    while (true)
    {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        uint16_t days_in_year = leap ? 366 : 365;
        if (days < days_in_year) break;
        days -= days_in_year;
        year++;
    }

    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    uint8_t month = 1;
    for (int i = 0; i < 12; i++)
    {
        uint8_t dim = days_in_month[i];
        if (leap && i == 1) dim++;
        if (days < dim) break;
        days -= dim;
        month++;
    }

    uint8_t day = days + 1;

    // === Step 3: Assign to structure ===
    current_time.year        = year;
    current_time.month       = month;
    current_time.day         = day;
    current_time.millisecond = ms_offset;
    current_time.offset_ms   = 0;

    global_time = current_time;
    base_millis_at_sync = ms;  // capture sync time for drift tracking

    Serial.println("[NTP] Time sync successful:");
    global_time.print();

    return true;
}

// Synchronize offset via RF protocol (placeholder for real implementation)
bool sync_time_rf(int32_t &offset_ms)
{
    offset_ms = 15; // example offset
    global_time.offset_ms = offset_ms;
    return true;
}

// Get current system time using global_time + (millis() - base_millis_at_sync)
NodeTime get_current_time()
{
    NodeTime now = global_time;

    // Milliseconds elapsed since last sync
    unsigned long elapsed_ms = millis() - base_millis_at_sync;
    unsigned long total_ms = global_time.millisecond + elapsed_ms;

    // Extract time units
    now.millisecond = total_ms % 1000;
    unsigned long total_sec = global_time.second + (total_ms / 1000);
    now.second = total_sec % 60;
    unsigned long total_min = global_time.minute + (total_sec / 60);
    now.minute = total_min % 60;
    unsigned long total_hour = global_time.hour + (total_min / 60);
    now.hour = total_hour % 24;
    unsigned long days_since_sync = (total_hour / 24);  // extra days passed

    // === Date calculation ===
    // Copy base date
    uint16_t y = global_time.year;
    uint8_t m = global_time.month;
    uint8_t d = global_time.day;

    // Helper array
    const uint8_t days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

    // Add extra days one by one
    while (days_since_sync > 0)
    {
        // Check days in this month
        uint8_t dim = days_in_month[m - 1];

        // February leap year adjustment
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
        {
            dim = 29;
        }

        if (d < dim)
        {
            d++;
            days_since_sync--;
        }
        else
        {
            d = 1;
            if (m < 12)
            {
                m++;
            }
            else
            {
                m = 1;
                y++;
            }
            days_since_sync--;
        }
    }

    now.year = y;
    now.month = m;
    now.day = d;

    return now;
}

int compare_node_time(const NodeTime &a, const NodeTime &b)
{
    #define CMP(field) if (a.field != b.field) return (a.field - b.field)
    CMP(year);
    CMP(month);
    CMP(day);
    CMP(hour);
    CMP(minute);
    CMP(second);
    CMP(millisecond);
    return 0;
}
