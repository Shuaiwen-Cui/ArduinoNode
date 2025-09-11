#pragma once
#include <Arduino.h>

/*
 * CalendarTime - Struct representing human-readable date and time.
 */
typedef struct
{
    /* === Calendar Fields === */
    uint16_t year;   // Year (e.g., 2025)
    uint8_t month;   // Month [1-12]
    uint8_t day;     // Day [1-31]
    uint8_t hour;    // Hour [0-23]
    uint8_t minute;  // Minute [0-59]
    uint8_t second;  // Second [0-59]
    int32_t ms;      // Milliseconds [0-999]
} CalendarTime;


CalendarTime calendar_from_unix_seconds(uint64_t unix_seconds);
CalendarTime calendar_from_unix_milliseconds(uint64_t unix_ms);
uint64_t unix_from_calendar_seconds(const CalendarTime &cal);
uint64_t unix_from_calendar_milliseconds(const CalendarTime &cal);
CalendarTime YYMMDDHHMMSS2Calendar(const char *datetime12);

/*
 * NodeTime - Unified time structure for embedded systems.
 * Provides both UNIX timestamp and human-readable calendar format.
 */
class NodeTime
{
public:
    /* === Running Time === */
    uint64_t running_time;             // Local running time in milliseconds since node startup

    /* === Time Tracking === */
    uint64_t last_sync_running_time;  // Running time when last sync occurred

    /* === Unified Time === */
    float drift_ratio;                // Clock drift ratio, applied as: adjusted = base * (1 + drift_ratio)
    uint64_t time_offset;             // Time offset in milliseconds for unified time correction
    uint64_t unified_time;            // Unified network time (in milliseconds)
    CalendarTime calendar_time;       // Human-readable calendar time

public:
    /* === Constructors === */
    NodeTime();

    /* === Setters === */
    void record_sync_time();

    /* === Getters === */
    uint64_t get_time();                // Get current unified time
    CalendarTime get_calendar();       // Get calendar time (stub for now, no RTC parsing)

    /* === Printout === */
    void show_time();
};

extern NodeTime Time;  // Global instance of NodeTime