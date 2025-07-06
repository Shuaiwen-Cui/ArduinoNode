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

