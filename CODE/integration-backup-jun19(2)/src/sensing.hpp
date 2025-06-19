#pragma once
#include <stdint.h>

// Initialize the MPU6050 sensor
void mpu_init();

// Prepare sampling: allocate memory and initialize state
bool sensing_prepare(uint16_t sample_rate_hz, uint16_t duration_sec);

// Set sensing flag on/off
void sensing_set_flag(bool enabled);

// Update logic to sample periodically
void sensing_update();

// Output metadata and all data
void sensing_flush();

// Return true if buffer full
bool sensing_is_full();
