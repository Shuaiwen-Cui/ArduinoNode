#pragma once

#include <stdint.h>

#define SENSING_PREPARING_DUR_MS 2000 // Duration for preparing sensing in milliseconds

extern uint64_t sensing_scheduled_start_ms; // Start time for sensing in milliseconds
extern uint64_t sensing_scheduled_end_ms;   // End time for sensing in milliseconds, default 10 seconds