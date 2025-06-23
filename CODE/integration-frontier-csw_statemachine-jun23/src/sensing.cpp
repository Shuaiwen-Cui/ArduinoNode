#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include "time.hpp"
#include "rgbled.hpp"
#include "mpu6050.hpp"
#include "sensing.hpp"
#include "mqtt.hpp"
#include "sdcard.hpp"
#include "logging.hpp"

uint64_t sensing_scheduled_start_ms = 0; // Start time for sensing in milliseconds
uint64_t sensing_scheduled_end_ms = 0;   // End time for sensing in milliseconds, default 10 seconds