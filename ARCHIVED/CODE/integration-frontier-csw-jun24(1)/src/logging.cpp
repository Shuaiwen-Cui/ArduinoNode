#include <Arduino.h>
#include <SD.h>
#include <stdint.h>
#include "logging.hpp"
#include "sdcard.hpp"

// === Log Number Tracker ===
int16_t log_number = 0;

// Load log number from SD card
void load_log_number()
{
    File logFile = SD.open("/LOG.txt", FILE_READ);
    if (logFile)
    {
        String line = logFile.readStringUntil('\n');
        logFile.close();
        if (line.startsWith("LOG_NUM = "))
        {
            log_number = line.substring(10).toInt();
        }
    }
    else
    {
        log_number = 0;
    }
}

// Save log number to SD card (overwrite)
void save_log_number()
{
    File logFile = SD.open("/LOG.txt", O_WRITE | O_CREAT | O_TRUNC);
    if (logFile)
    {
        logFile.print("LOG_NUM = ");
        logFile.println(log_number);
        logFile.close();
    }
    else
    {
        Serial.println("[SD] Failed to open LOG.txt for writing.");
    }
}