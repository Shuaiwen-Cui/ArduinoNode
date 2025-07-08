# TIME

Time management is an important research area in wireless sensor networks. It involves how to effectively coordinate and schedule nodes in the network to achieve efficient data collection, transmission, and processing. In this section, we will introduce some basic time management concepts, and time synchronization will be discussed in detail in other chapters.

Before we delve into detailed discussion, let's first investigate the related codes:

**time.hpp**

```cpp
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
    bool set_from_string_YYMMDDHHMMSS(const char *datetime12);


    /* === Comparison === */
    int8_t compare_to(const MCUTime &other) const;
};

// Add at the bottom of the header
extern MCUTime Time;  // Global time variable
extern MCUTime parsed_start_time; // Parsed start time for sensing commands
extern MCUTime SensingSchedule; // Time synchronization variable

```

**time.cpp**

```cpp
#include "time.hpp"

/* Definition of the operator functions in the MCUTime class */
MCUTime::MCUTime()
{
    unix_epoch = 0;
    unix_ms = 0;
    estimated_unix_ms = 0;
    estimated_unix_epoch = 0;
    year = 1970;
    month = 1;
    day = 1;
    hour = 0;
    minute = 0;
    second = 0;
    ms = 0;
    last_update_ms = 0;
    last_update_epoch = 0;
    mcu_time_ms = 0;
    delta_ms = 0;
}

// Helper: check leap year
static bool is_leap_year(uint16_t year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

// Helper: days in each month (non-leap year)
static const uint8_t days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31};

void MCUTime::set_calendar()
{
    // Extract milliseconds
    ms = unix_ms % 1000;

    // Get total seconds
    uint64_t seconds = unix_ms / 1000;

    // Keep a backup in the object
    unix_epoch = seconds;

    // Calculate time-of-day
    second = seconds % 60;
    seconds /= 60;
    minute = seconds % 60;
    seconds /= 60;
    hour = seconds % 24;
    seconds /= 24;

    // Now calculate date from days since 1970-01-01
    uint32_t days = (uint32_t)seconds;
    year = 1970;

    while (true)
    {
        uint16_t days_in_year = is_leap_year(year) ? 366 : 365;
        if (days >= days_in_year)
        {
            days -= days_in_year;
            year++;
        }
        else
        {
            break;
        }
    }

    // Now determine the month and day
    uint8_t month_index = 0;
    while (true)
    {
        uint8_t dim = days_in_month[month_index];
        if (month_index == 1 && is_leap_year(year))
            dim++; // February in leap year
        if (days >= dim)
        {
            days -= dim;
            month_index++;
        }
        else
        {
            break;
        }
    }

    month = month_index + 1;
    day = days + 1;
}

void MCUTime::set_time_ms(uint64_t ms)
{
    unix_ms = ms;
    unix_epoch = ms / 1000;

    // Update calendar fields
    set_calendar();
}

// be very careful to use this function
void MCUTime::set_time_epoch(uint64_t epoch)
{
    unix_epoch = epoch;
    unix_ms = epoch * 1000 + (millis() % 1000); // Convert to ms, keeping current millis for ms part

    // Update calendar fields
    set_calendar();
}

uint64_t MCUTime::estimate_time_ms()
{
    // Calculate delta since last update
    mcu_time_ms = millis();
    delta_ms = mcu_time_ms - mcu_base_ms;
    estimated_unix_ms = last_update_ms + delta_ms;
    estimated_unix_epoch = last_update_epoch + (delta_ms / 1000);

    return estimated_unix_ms;
}

uint64_t MCUTime::estimate_time_epoch()
{
    // Calculate delta since last update
    mcu_time_ms = millis();
    delta_ms = mcu_time_ms - mcu_base_ms;

    estimated_unix_ms = last_update_ms + delta_ms;
    estimated_unix_epoch = last_update_epoch + (delta_ms / 1000);

    return estimated_unix_epoch;
}

uint64_t MCUTime::get_unix_ms() const
{
    return unix_ms;
}

uint64_t MCUTime::get_unix_epoch() const
{
    return unix_epoch;
}

uint64_t MCUTime::get_now_time_ms() const
{
    return millis();
}

uint64_t MCUTime::compute_ms_from_calendar() const
{
    uint64_t days = 0;

    for (uint16_t y = 1970; y < year; y++)
    {
        days += is_leap_year(y) ? 366 : 365;
    }

    for (uint8_t m = 1; m < month; m++)
    {
        days += days_in_month[m - 1];
        if (m == 2 && is_leap_year(year))
            days++;
    }

    days += (day - 1);

    uint64_t ret_ms = (days * 86400ULL + hour * 3600 + minute * 60 + second) * 1000;
    ret_ms += ms;  // Include milliseconds

    return ret_ms;
}


String MCUTime::to_string() const
{
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u.%03d",
             year, month, day, hour, minute, second, ms);
    return String(buffer);
}

void MCUTime::print() const
{
    Serial.println(to_string());
}

bool MCUTime::set_from_string_YYMMDDHHMMSS(const char *datetime12)
{
    if (strlen(datetime12) != 12)
        return false;

    char buf[3] = {0};

    strncpy(buf, datetime12, 2); year   = 2000 + atoi(buf);  // YY → 20YY
    strncpy(buf, datetime12 + 2, 2); month  = atoi(buf);
    strncpy(buf, datetime12 + 4, 2); day    = atoi(buf);
    strncpy(buf, datetime12 + 6, 2); hour   = atoi(buf);
    strncpy(buf, datetime12 + 8, 2); minute = atoi(buf);
    strncpy(buf, datetime12 + 10,2); second = atoi(buf);

    ms = 0;
    return true;
}

int8_t MCUTime::compare_to(const MCUTime &other) const
{
    if (unix_ms < other.unix_ms)
        return -1; // this is earlier
    else if (unix_ms > other.unix_ms)
        return 1; // this is later
    else
        return 0; // they are equal
}

/* Global time variable */
MCUTime Time;

/* Command Parsing */
MCUTime parsed_start_time;

/* Sensing Config */
MCUTime SensingSchedule;

```

## Time Representation

### Natural Timekeeping

Natural timekeeping refers to using calendar and clock formats familiar to humans to represent time. This usually includes fields such as year, month, day, hour, minute, and second. While this format is easy for humans to understand and use, it may not be efficient for computer systems to process.

### Unix Time

Unix time refers to the number of seconds that have elapsed since January 1, 1970, 00:00:00 UTC. It is a standard time representation widely used in computer systems. The advantage of Unix time is that it is easy to compute and compare, although it is not easily readable by humans. Network time synchronization typically uses Unix time to align system clocks.

### Runtime

Runtime refers to the duration from system startup to the current moment, usually measured in milliseconds. It is particularly important in real-time systems and embedded devices, as it helps monitor system status and performance. Most hardware platforms provide a timer to track runtime.

!!! note "Note"
    In practical applications, natural timekeeping and Unix time are often converted between each other. Natural time is used when human-readable output is needed, while Unix time is preferred for efficient computation. In this project, while high precision is not strictly required, we adopt millisecond-level time representation to ensure reliable functionality.

!!! tip "Summary"
    From the perspective of time representation, all three methods have their strengths and weaknesses. Natural time is human-friendly but inefficient for machines; Unix time is computation-friendly but hard to read; runtime offers high precision but is relative to system startup time. For wireless sensor networks, synchronization between nodes is often needed.

    To meet these requirements, we define a unified time structure called `MCUTime` in the `time` module. It includes fields for Unix time, natural time, and runtime, along with helper functions to support time conversion and calculation.

To facilitate time management, we define three key time variables:

- `Time`: A global time variable that represents the current system time.

- `parsed_start_time`: A variable used to parse the start time from commands, allowing users to specify the start time for sampling in commands.

- `SensingSchedule`: A configured sampling time variable used to record the scheduled sensing start time.
