#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"

/*
 * Time synchronization header
 * 
 * Provides:
 * - NTP synchronization function
 * - Drift Ratio Calculation
 * - Offset Calculation
 */

bool sync_time_ntp();