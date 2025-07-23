#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"

#define SYNC_ROUNDS 6
#define SYNC_INTERVAL_1 10000
#define SYNC_INTERVAL_N 5000

/*
 * Time synchronization header
 * 
 * Provides:
 * - NTP synchronization function
 * - RF time synchronization function by drift ratio and offset
 */

bool sync_time_ntp();
bool rf_time_sync();