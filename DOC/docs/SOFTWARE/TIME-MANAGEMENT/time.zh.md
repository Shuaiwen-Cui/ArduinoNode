# 时间

时间管理在无线传感器网络中是一个重要的研究领域。它涉及到如何有效地协调和调度网络中的节点，以实现高效的数据采集、传输和处理。本节我们将介绍一些基础的时间管理概念，至于时间同步将会在其他章节中详细讨论。

在讨论具体概念之前，我们先来看一下相关的代码

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

## 时间的表示

### 自然记时

自然记时是指使用人类熟悉的日历和时钟格式来表示时间。它通常包括年、月、日、小时、分钟和秒等字段。这种方式易于理解和使用，但在计算机系统中处理起来可能不够高效。

### Unix时间

Unix时间是指自1970年1月1日00:00:00 UTC以来的秒数。它是一种标准的时间表示方式，广泛用于计算机系统中。Unix时间的优点是易于计算和比较，但不易于人类阅读。网络对时通常使用Unix时间来同步系统时间。

### 运行时间

运行时间是指从系统启动到当前时刻的时间长度，通常以毫秒为单位。它对于实时系统和嵌入式设备非常重要，因为它可以帮助我们了解系统的运行状态和性能。通常硬件平台会提供一个计时器来跟踪运行时间。

!!! note "注意"
    在实际应用中，通常会将自然记时和Unix时间进行转换，以便在需要人类可读格式时使用自然记时，而在需要高效计算时使用Unix时间。本项目中，我们不需要特别高的精度，但是为了保证功能的可靠性，我们需要使用毫秒级的时间表示。

!!! tip "总结"
    从时间的表示上我们可以看到，三中方式各有优缺点。自然记时易于人类理解，但计算机处理起来不够高效；Unix时间便于计算和比较，但不易于人类阅读；运行时间精度高，但是它是相对于系统启动时间的，对于无线传感器网络来说，通常需要与其他节点进行同步。为了满足这些需求，我们在time模块中定义了一个统一的时间结构`NodeTime`，它包含了Unix时间、自然记时和运行时间的相关字段，也包含了一些辅助函数来进行时间的转换和计算。

为了方便计算，我们定义了三个个时间变量：

- `Time`：全局时间变量，用于表示当前系统时间。