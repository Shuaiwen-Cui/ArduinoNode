#pragma once
#include <SD.h>
#include <Arduino.h>

/**
 * @brief Initialize the SD card with the specified CS pin.
 * 
 * @param cs_pin Chip select pin. Default is 10.
 * @return true if initialization succeeds, false otherwise.
 */
bool sdcard_init(uint8_t cs_pin = 10);

/**
 * @brief Perform a test: write a text to file, then read it back.
 * 
 * Writes "Hello from Arduino!" to "test.txt", then reads and prints its content.
 */
void sdcard_test_fileio();
