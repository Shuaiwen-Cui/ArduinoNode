#include "sensing.hpp"
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <SD.h>
#include "mqtt.hpp"
#include "config.hpp"
#include "time.hpp"

// === Constants ===
static const uint32_t max_memory_bytes = 22000;
static const uint8_t channels = 4;

// === Sampling Parameters ===
static uint16_t sample_rate = 100;
static uint16_t duration_sec = 10;
static uint32_t total_samples = 0;
static uint32_t sample_interval_ms = 10;

// === Sampling State ===
static int16_t *sample_data = nullptr;
static uint32_t sample_index = 0;
static bool sampling_flag = false;
static unsigned long last_sample_time = 0;
static unsigned long start_millis = 0;
static NodeTime start_time;

// === Scheduled Sampling State ===
bool scheduled_sampling_enabled = false;
NodeTime scheduled_start_time;

// === Log Number Tracker ===
int16_t log_number = 0;

// === MPU6050 Initialization ===
static MPU6050 mpu(0x68);

void mpu_init()
{
    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection())
    {
        Serial.println("[MPU6050] Connection failed");
        while (1)
            ;
    }
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    Serial.println("[MPU6050] Connected");
}

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

// === Sampling Configuration ===
bool sensing_prepare(const NodeTime &start_time_input, uint16_t rate_hz, uint16_t dur_sec)
{
    load_log_number();

    sample_rate = rate_hz;
    duration_sec = dur_sec;
    total_samples = sample_rate * duration_sec;
    sample_interval_ms = 1000 / sample_rate;
    uint32_t needed_bytes = total_samples * channels * sizeof(int16_t);

    if (needed_bytes > max_memory_bytes)
    {
        Serial.print("[Error] Memory limit exceeded. Need ");
        Serial.print(needed_bytes);
        Serial.println(" bytes.");
        mqtt_enabled = true;
        return false;
    }

    if (sample_data)
    {
        free(sample_data);
        sample_data = nullptr;
    }

    sample_data = (int16_t *)malloc(needed_bytes);
    if (!sample_data)
    {
        Serial.println("[Error] malloc failed");
        mqtt_enabled = true;
        return false;
    }

    sample_index = 0;

    NodeTime now = get_current_time();
    if (compare_node_time(start_time_input, now) <= 0)
    {
        Serial.println("[Sensing] Start time is now or in the past. Start immediately.");
        sensing_set_flag(true);
        scheduled_sampling_enabled = false;
    }
    else
    {
        scheduled_start_time = start_time_input;
        scheduled_sampling_enabled = true;
        Serial.print("[Sensing] Scheduled to start at: ");
        scheduled_start_time.print();
    }

    Serial.print("[Sensing] Sampling configured: ");
    Serial.print(sample_rate);
    Serial.print(" Hz for ");
    Serial.print(duration_sec);
    Serial.println(" sec");

    return true;
}

void sensing_set_flag(bool enabled)
{
    sampling_flag = enabled;

    if (enabled)
    {
        mqtt_enabled = false;
        sample_index = 0;
        start_millis = millis();
        last_sample_time = 0;
        start_time = get_current_time();
    }
}

bool sensing_is_full()
{
    if (!sample_data)
        return false;
    return sample_index >= total_samples;
}

void sensing_sample()
{
    if (!sample_data || sensing_is_full())
        return;

    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    uint16_t dt_ms = (uint16_t)(millis() - start_millis);

    uint32_t base = sample_index * channels;
    sample_data[base + 0] = dt_ms;
    sample_data[base + 1] = ax;
    sample_data[base + 2] = ay;
    sample_data[base + 3] = az;
    sample_index++;
}

void sensing_update()
{
    if (scheduled_sampling_enabled && !sampling_flag)
    {
        NodeTime now = get_current_time();
        if (compare_node_time(now, scheduled_start_time) >= 0)
        {
            Serial.println("[Sensing] Scheduled time reached. Start sampling.");
            sensing_set_flag(true);
            scheduled_sampling_enabled = false;
        }
        return;
    }

    if (!sampling_flag || sensing_is_full())
        return;

    unsigned long now = millis();
    if (now - last_sample_time >= sample_interval_ms)
    {
        last_sample_time = now;
        sensing_sample();
    }
}

void sensing_flush()
{
    if (!sample_data)
    {
        Serial.println("[Warning] No data to flush.");
        mqtt_enabled = true;
        return;
    }

    // Print to Serial
    Serial.println("=== Sensing Metadata ===");

    char time_str[32];
    snprintf(time_str, sizeof(time_str), "Start Time (Base): %04d-%02d-%02d %02d:%02d:%02d",
             start_time.year, start_time.month, start_time.day,
             start_time.hour, start_time.minute, start_time.second);
    Serial.println(time_str);

    Serial.print("Sample Rate: ");
    Serial.print(sample_rate);
    Serial.println(" Hz");

    Serial.print("Duration: ");
    Serial.print((float)sample_index / sample_rate, 2);
    Serial.println(" sec");

    Serial.print("Total Samples: ");
    Serial.println(sample_index);

    Serial.print("Time Offset: ");
    Serial.print(start_time.offset_ms);
    Serial.println(" ms");

    // Construct filename
    char filename[20];
    snprintf(filename, sizeof(filename),
             "/N%03d_%03d.txt",
             NODE_ID, log_number + 1);

    Serial.print("[SD] Attempting to open file: ");
    Serial.println(filename);

    File file = SD.open(filename, FILE_WRITE);
    if (file)
    {
        // Metadata header
        file.println("==============");
        file.print("Start Time (Base): ");
        file.println(time_str);

        file.print("Time Offset (ms): ");
        file.println(start_time.offset_ms);

        file.print("Sample Rate (Hz): ");
        file.println(sample_rate);

        file.print("Duration (s): ");
        file.println(duration_sec);

        file.println("==============");
        file.println("time_ms,ax,ay,az");

        // Write data
        for (uint32_t i = 0; i < sample_index; ++i)
        {
            uint32_t base = i * channels;
            uint16_t t_ms = sample_data[base + 0];
            float ax = sample_data[base + 1] / 16384.0f;
            float ay = sample_data[base + 2] / 16384.0f;
            float az = sample_data[base + 3] / 16384.0f;

            char line[64];
            snprintf(line, sizeof(line), "%05u,%8.6f,%8.6f,%8.6f", t_ms, ax, ay, az);
            file.println(line);
            Serial.println(line);  // Debug output to Serial
        }

        file.close();
        Serial.print("[SD] Data saved to ");
        Serial.println(filename);

        log_number++;
        save_log_number();
    }
    else
    {
        Serial.println("[SD] Failed to open file for writing.");
    }

    Serial.println("====================");

    free(sample_data);
    sample_data = nullptr;
    sample_index = 0;
    mqtt_enabled = true;
}

