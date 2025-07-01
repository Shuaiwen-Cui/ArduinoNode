#pragma once

#include <stdint.h>

#define SENSING_PREPARING_DUR_MS 5000  // Duration for preparing sensing in milliseconds

extern uint64_t sensing_scheduled_start_ms; // Scheduled sensing start time (Unix ms)
extern uint64_t sensing_scheduled_end_ms;   // Scheduled sensing end time (Unix ms)
extern uint32_t sensing_rate_hz;            // Sensing rate in Hz
extern uint32_t sensing_duration_s;         // Sensing duration in seconds

typedef struct {
    uint16_t elapsed_ms;  // Elapsed time since sensing started (ms)
    int16_t ax;
    int16_t ay;
    int16_t az;
} SamplePoint;

bool sensing_start();                       // Called once at the beginning of SAMPLING state
void sensing_sample_once();                 // Called repeatedly during SAMPLING state
void sensing_stop();                        // Called once at the end of SAMPLING state

void sensing_retrieve_file();               // Retrieve file from SD card
