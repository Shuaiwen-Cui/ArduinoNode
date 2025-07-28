#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"

#define SYNC_ROUNDS 7
#define SYNC_INTERVAL_1 20000
#define SYNC_INTERVAL_N 2000
#define TIME_SYNC_RESERVED_TIME 60000 // means reserve at least 60 seconds for time sync when issuing a sensing command

/*
 * Time synchronization header
 * 
 * Provides:
 * - NTP synchronization function
 * - RF time synchronization function by drift ratio and offset
 */

bool sync_time_ntp();
bool rf_time_sync();