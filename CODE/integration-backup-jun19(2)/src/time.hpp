#pragma once

#include <stdint.h>

// Structure to represent full node timestamp with relative offset
struct NodeTime
{
    uint16_t year;         // Year (e.g., 2025)
    uint8_t month;         // Month (1–12)
    uint8_t day;           // Day (1–31)
    uint8_t hour;          // Hour (0–23)
    uint8_t minute;        // Minute (0–59)
    uint8_t second;        // Second (0–59)
    uint16_t millisecond;  // Millisecond (0–999)
    int32_t offset_ms;     // Time offset in milliseconds relative to reference node
    void print() const;    // Print time information to serial
};

// === Global variables ===
extern NodeTime global_time;            // Reference time when last sync occurred
extern unsigned long base_millis_at_sync; // System millis() at time of last sync

// Initialize the time system (reset global_time)
void time_init();

// Synchronize time from NTP server (sets global_time and base_millis_at_sync)
bool sync_time_ntp(NodeTime &current_time);

// Synchronize offset using RF-based sync (optional)
bool sync_time_rf(int32_t &offset_ms);

// Get current system time (global_time + millis offset)
NodeTime get_current_time();

