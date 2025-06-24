#pragma once
#include <Arduino.h>

/*
 * MCUTime - Unified time structure for embedded systems.
 * Provides both UNIX timestamp and human-readable calendar format.
 */
class MCUTime
{
public:
    /* === UNIX Time Fields === */
    uint64_t unix_epoch; // Seconds since 1970-01-01 00:00:00 UTC
    uint64_t unix_ms;    // Milliseconds since 1970-01-01 00:00:00 UTC
    uint64_t estimated_unix_ms; // Estimated milliseconds since 1970-01-01 00:00:00 UTC
    uint64_t estimated_unix_epoch; // Estimated seconds since 1970-01-01 00:00:00 UTC

    /* === Calendar Fields === */
    uint16_t year;  // Year (e.g., 2025)
    uint8_t month;  // Month [1-12]
    uint8_t day;    // Day [1-31]
    uint8_t hour;   // Hour [0-23]
    uint8_t minute; // Minute [0-59]
    uint8_t second; // Second [0-59]
    int32_t ms;     // Milliseconds [0-999]

    /* === Time Tracking === */
    uint64_t last_update_ms; // Last update time in milliseconds by syncing
    uint64_t last_update_epoch; // Last update time in seconds since epoch by syncing
    uint64_t mcu_time_ms; // Current time in milliseconds since MCU start
    uint64_t mcu_base_ms; // Base time in milliseconds when synced
    uint64_t delta_ms; // Time since last update in milliseconds

public:
    /* === Constructors === */
    MCUTime(); // Default constructor initializes to 0

    /* === Setters === */
    void set_calendar();  // Set calendar fields based on current Unix time
    void set_time_ms(uint64_t ms); // Set Unix time by passing milliseconds since Unix epoch
    void set_time_epoch(uint64_t epoch); // Set Unix time by passing seconds since Unix epoch

    /* === Time Update === */
    uint64_t estimate_time_ms(); // estimate time ms based on MCU millis and last update
    uint64_t estimate_time_epoch(); // estimate time epoch based on MCU millis and last update

    /* === Getters === */
    uint64_t get_unix_ms() const;
    uint64_t get_unix_epoch() const;
    uint64_t get_now_time_ms() const; // Get current time in ms in Unix ms format
    uint64_t compute_ms_from_calendar() const; // Compute Unix ms from calendar fields
    String to_string() const; // Return "YYYY-MM-DD HH:MM:SS.mmm"
    void print() const;       // Print formatted string to Serial

    /* === Comparison === */
    int8_t compare_to(const MCUTime &other) const;
};

// Add at the bottom of the header
extern MCUTime Time;  // Global time variable
extern MCUTime SensingSchedule; // Time synchronization variable