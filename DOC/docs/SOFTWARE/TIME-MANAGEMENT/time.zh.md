# 时间

时间管理在无线传感器网络中是一个重要的研究领域。它涉及到如何有效地协调和调度网络中的节点，以实现高效的数据采集、传输和处理。本节我们将介绍一些基础的时间管理概念，至于时间同步将会在其他章节中详细讨论。

在讨论具体概念之前，我们先来看一下相关的代码

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
    从时间的表示上我们可以看到，三中方式各有优缺点。自然记时易于人类理解，但计算机处理起来不够高效；Unix时间便于计算和比较，但不易于人类阅读；运行时间精度高，但是它是相对于系统启动时间的，对于无线传感器网络来说，通常需要与其他节点进行同步。为了满足这些需求，我们在time模块中定义了一个统一的时间结构`MCUTime`，它包含了Unix时间、自然记时和运行时间的相关字段，也包含了一些辅助函数来进行时间的转换和计算。

为了方便计算，我们定义了三个个时间变量：

- `Time`：全局时间变量，用于表示当前系统时间。

- `parsed_start_time`：用于解析命令中的开始时间，方便在命令中指定采样的起始时间。

- `SensingSchedule`：设定采样时间变量，用于记录设定的采样时间。