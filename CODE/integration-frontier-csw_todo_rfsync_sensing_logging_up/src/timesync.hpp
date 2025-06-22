#pragma once

#include <Arduino.h>

/* 
 * Time synchronization via NTP.
 * This module updates the global Time object.
 */

// Try to synchronize time via NTP server (e.g., pool.ntp.org)
// Returns true on success, false otherwise.
bool sync_time_ntp();
