# TIME

Time management is an important research area in wireless sensor networks. It involves how to effectively coordinate and schedule nodes in the network to achieve efficient data collection, transmission, and processing. In this section, we will introduce some basic time management concepts, and time synchronization will be discussed in detail in other chapters.

Before we delve into detailed discussion, let's first investigate the related codes:

**time.hpp**

```cpp
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
```

**time.cpp**

```cpp
#include "time.hpp"

/* === Helper Functions === */
CalendarTime calendar_from_unix_seconds(uint64_t unix_seconds)
{
    CalendarTime cal;
    uint64_t seconds = unix_seconds;

    cal.second = seconds % 60;
    seconds /= 60;
    cal.minute = seconds % 60;
    seconds /= 60;
    cal.hour = seconds % 24;
    seconds /= 24; // Total days since epoch

    int year = 1970;
    while (true)
    {
        bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        int days_in_year = is_leap ? 366 : 365;
        if (seconds >= days_in_year)
        {
            seconds -= days_in_year;
            year++;
        }
        else
        {
            break;
        }
    }
    cal.year = year;

    static const uint8_t days_in_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31};

    int month = 0;
    while (month < 12)
    {
        int dim = days_in_month[month];
        if (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
            dim = 29;

        if (seconds >= dim)
        {
            seconds -= dim;
            month++;
        }
        else
        {
            break;
        }
    }
    cal.month = month + 1;
    cal.day = seconds + 1;
    cal.ms = 0;

    return cal;
}

CalendarTime calendar_from_unix_milliseconds(uint64_t unix_ms)
{
    CalendarTime cal = calendar_from_unix_seconds(unix_ms / 1000);
    cal.ms = unix_ms % 1000;
    return cal;
}

uint64_t unix_from_calendar_seconds(const CalendarTime &cal)
{
    uint64_t days = 0;

    for (int y = 1970; y < cal.year; ++y)
    {
        bool is_leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
        days += is_leap ? 366 : 365;
    }

    static const uint8_t days_in_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31};

    for (int m = 0; m < cal.month - 1; ++m)
    {
        if (m == 1 && (cal.year % 4 == 0 && (cal.year % 100 != 0 || cal.year % 400 == 0)))
            days += 29;
        else
            days += days_in_month[m];
    }

    days += (cal.day - 1);

    return days * 86400ULL + cal.hour * 3600 + cal.minute * 60 + cal.second;
}

uint64_t unix_from_calendar_milliseconds(const CalendarTime &cal)
{
    return unix_from_calendar_seconds(cal) * 1000ULL + cal.ms;
}

// Convert YYMMDDHHMMSS string to CalendarTime structure
CalendarTime YYMMDDHHMMSS2Calendar(const char *datetime12)
{
    CalendarTime ct = {0};

    if (strlen(datetime12) != 12) {
        // Return a zero-initialized CalendarTime on failure
        return ct;
    }

    char buf[3] = {0};

    // Extract and convert each part of the datetime string
    strncpy(buf, datetime12, 2);
    ct.year = 2000 + atoi(buf);  // YY → 20YY
    strncpy(buf, datetime12 + 2, 2);
    ct.month = atoi(buf);
    strncpy(buf, datetime12 + 4, 2);
    ct.day = atoi(buf);
    strncpy(buf, datetime12 + 6, 2);
    ct.hour = atoi(buf);
    strncpy(buf, datetime12 + 8, 2);
    ct.minute = atoi(buf);
    strncpy(buf, datetime12 + 10, 2);
    ct.second = atoi(buf);

    ct.ms = 0;  // Milliseconds are set to 0 as per the original function

    return ct;
}

/* === Major Functions === */
NodeTime::NodeTime()
{
    running_time = 0;
    last_sync_running_time = 0;
    drift_ratio = 1.0f; // Default drift ratio (no drift)
    time_offset = 0;    // Default time offset
    unified_time = 0;
    calendar_time = {0, 0, 0, 0, 0, 0, 0}; // Initialize calendar time to zero
}

void NodeTime::record_sync_time()
{
    last_sync_running_time = millis(); // Record the current running time
}



// uint64_t NodeTime::get_time()
// {
//     running_time = millis(); // Get the current running time in milliseconds
//     unified_time = static_cast<uint64_t>((drift_ratio * (running_time - last_sync_running_time)) + time_offset);
//     return unified_time;
// }

// uint64_t NodeTime::get_time()
// {
//     running_time = millis();

//     // Use double to preserve precision during multiplication
//     double delta = static_cast<double>(running_time - last_sync_running_time);
//     return static_cast<uint64_t>(delta * drift_ratio + static_cast<double>(time_offset));
// }

// uint64_t NodeTime::get_time()
// {
//     running_time = millis();

//     // Use double to preserve precision during multiplication
//     double delta = static_cast<double>(running_time - last_sync_running_time);
    
//     // Include last_sync_running_time in the return value
//     double adjusted_time = delta * drift_ratio + static_cast<double>(time_offset);

//     // Return the result as uint64_t
//     return static_cast<uint64_t>(adjusted_time);
// }


uint64_t NodeTime::get_time()
{
    running_time = millis();

    // Use double to preserve precision during multiplication
    double delta = static_cast<double>(running_time - last_sync_running_time);
    
    // Include last_sync_running_time in the return value
    double adjusted_time = static_cast<double>(last_sync_running_time) + delta * drift_ratio + static_cast<double>(time_offset);

    // Return the result as uint64_t
    return static_cast<uint64_t>(adjusted_time);
}

CalendarTime NodeTime::get_calendar()
{
    uint64_t current_time = get_time(); // Milliseconds since 1970-01-01 00:00:00 UTC
    uint64_t ms_total = current_time;

    calendar_time.ms = ms_total % 1000;
    time_t seconds = ms_total / 1000;

    // === Extract time ===
    calendar_time.second = seconds % 60;
    seconds /= 60;
    calendar_time.minute = seconds % 60;
    seconds /= 60;
    calendar_time.hour = seconds % 24;
    seconds /= 24; // Now we have total days since epoch

    // === Extract date ===
    int year = 1970;
    while (true)
    {
        bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        int days_in_year = is_leap ? 366 : 365;
        if (seconds >= days_in_year)
        {
            seconds -= days_in_year;
            year++;
        }
        else
        {
            break;
        }
    }
    calendar_time.year = year;

    static const uint8_t days_in_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31};

    int month = 0;
    while (month < 12)
    {
        int dim = days_in_month[month];

        // Adjust for leap year in February
        if (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
            dim = 29;

        if (seconds >= dim)
        {
            seconds -= dim;
            month++;
        }
        else
        {
            break;
        }
    }
    calendar_time.month = month + 1; // [1-12]
    calendar_time.day = seconds + 1; // [1-31]

    return calendar_time;
}

void NodeTime::show_time()
{
    // Update current times
    running_time = millis();
    uint64_t now = get_time();

    // Use unified function for calendar conversion
    CalendarTime cal = calendar_from_unix_milliseconds(now);

    Serial.println("=== Node Time Info ===");

    Serial.print("Running Time      : ");
    Serial.print(running_time);
    Serial.println(" ms");

    Serial.print("Unified Time      : ");
    Serial.print(now);
    Serial.println(" ms");

    Serial.print("Calendar Time     : ");
    Serial.print(cal.year);
    Serial.print("-");
    if (cal.month < 10)
        Serial.print("0");
    Serial.print(cal.month);
    Serial.print("-");
    if (cal.day < 10)
        Serial.print("0");
    Serial.print(cal.day);
    Serial.print(" ");

    if (cal.hour < 10)
        Serial.print("0");
    Serial.print(cal.hour);
    Serial.print(":");
    if (cal.minute < 10)
        Serial.print("0");
    Serial.print(cal.minute);
    Serial.print(":");
    if (cal.second < 10)
        Serial.print("0");
    Serial.print(cal.second);
    Serial.print(".");
    if (cal.ms < 100)
        Serial.print("0");
    if (cal.ms < 10)
        Serial.print("0");
    Serial.println(cal.ms);

    Serial.println("======================");
}

NodeTime Time;



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

    To meet these requirements, we define a unified time structure called `NodeTime` in the `time` module. It includes fields for Unix time, natural time, and runtime, along with helper functions to support time conversion and calculation.

To facilitate time management, we define three key time variables:

- `Time`: A global time variable that represents the current system time.