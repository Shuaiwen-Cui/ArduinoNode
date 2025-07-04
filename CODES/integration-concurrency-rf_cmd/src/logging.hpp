#pragma once

extern int16_t log_number; // Current log number

// Load or save the log number from/to SD card
void load_log_number();
void save_log_number();