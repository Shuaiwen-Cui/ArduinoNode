#pragma once

#include <stdint.h>

#define SENSING_PREPARING_DUR_MS 5000  // Duration for preparing sensing in milliseconds

typedef struct {
    uint16_t elapsed_ms;  // Elapsed time since sensing started (ms)
    int16_t ax;
    int16_t ay;
    int16_t az;
} SamplePoint;

bool sensing_prepare();                     // Called once at the beginning of PREPARING state
void sensing_sample_once();                 // Called repeatedly during SAMPLING state
void sensing_stop();                        // Called once at the end of SAMPLING state

void sensing_retrieve_file();               // Retrieve file from SD card
