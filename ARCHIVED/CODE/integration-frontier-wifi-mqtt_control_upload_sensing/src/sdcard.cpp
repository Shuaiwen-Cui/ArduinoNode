#include "sdcard.hpp"

static bool sd_initialized = false;

bool sdcard_init(uint8_t cs_pin)
{
    if (!SD.begin(cs_pin))
    {
        Serial.println("[SD] Initialization failed!");
        sd_initialized = false;
        return false;
    }

    Serial.println("[SD] Initialization succeeded.");
    sd_initialized = true;
    return true;
}

void sdcard_test_fileio()
{
    if (!sd_initialized)
    {
        Serial.println("[SD] Not initialized. Skipping File IO.");
        return;
    }

    // Write to file
    File dataFile = SD.open("test.txt", FILE_WRITE);
    if (dataFile)
    {
        dataFile.println("Hello from Arduino!");
        dataFile.close();
        Serial.println("[SD] Data written to test.txt");
    }
    else
    {
        Serial.println("[SD] Failed to open file for writing.");
        return;
    }

    // Read from file
    dataFile = SD.open("test.txt");
    if (dataFile)
    {
        Serial.println("[SD] Reading from test.txt:");
        while (dataFile.available())
        {
            Serial.write(dataFile.read());
        }
        dataFile.close();
    }
    else
    {
        Serial.println("[SD] Failed to open file for reading.");
    }
}
