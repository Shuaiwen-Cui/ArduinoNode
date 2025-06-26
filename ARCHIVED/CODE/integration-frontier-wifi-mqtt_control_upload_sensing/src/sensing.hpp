#pragma once

#include <stdint.h>
#include "time.hpp"

// === MPU6050 Setup ===
void mpu_init();

// Load or save the log number from/to SD card
void load_log_number();
void save_log_number();

// === Sensing Configuration ===
bool sensing_prepare(const NodeTime &start_time, uint16_t rate_hz, uint16_t dur_sec);
void sensing_set_flag(bool enabled);

// === Sensing Execution ===
void sensing_sample();
void sensing_update();
void sensing_flush();
bool sensing_is_full();

// === Scheduled Sampling State ===
extern bool scheduled_sampling_enabled;
extern NodeTime scheduled_start_time;
