#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

// Initialize MPU6050
void imu_init();

// Read raw 3-axis acceleration values
void imu_get_acceleration(int16_t &ax, int16_t &ay, int16_t &az);
