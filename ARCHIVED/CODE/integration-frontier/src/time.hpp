#pragma once

#include <stdint.h>

struct NodeTime
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    int32_t offset_ms;

    void print() const;
};

extern NodeTime global_time;

void time_init();
bool sync_time_ntp(NodeTime &current_time);
bool sync_time_rf(int32_t &offset_ms);
NodeTime get_current_time();

// Compare two NodeTime objects: <0 if a<b, =0 if a==b, >0 if a>b
int compare_node_time(const NodeTime &a, const NodeTime &b);
