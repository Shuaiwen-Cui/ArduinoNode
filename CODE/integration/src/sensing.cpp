#include "sensing.hpp"
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include "time.hpp"
#include "mqtt.hpp"

static MPU6050 mpu(0x68);

// Sampling config
static uint16_t sample_rate = 100; // default value, can be modified by calling sensing_prepare()
static uint16_t duration_sec = 10; // default value, can be modified by calling sensing_prepare()
static uint32_t total_samples = 0; // total samples to collect, automatically calculated from sample_rate and duration_sec
static uint32_t sample_interval_ms = 10; // interval between samples in milliseconds, calculated as 1000 / sample_rate

static const uint32_t max_memory_bytes = 22000; // this value is obtained by testing the maximum memory available on the device
static const uint8_t channels = 4; // time, ax, ay, az

// Data buffer
static int16_t *sample_data = nullptr;
static uint32_t sample_index = 0;

// Sampling state
static bool sampling_flag = false;
static unsigned long last_sample_time = 0;
static unsigned long start_millis = 0;
static NodeTime start_time;

void mpu_init()
{
    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection())
    {
        Serial.println("[MPU6050] Connection failed");
        while (1);
    }
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    Serial.println("[MPU6050] Connected");
}

bool sensing_prepare(uint16_t rate_hz, uint16_t dur_sec)
{
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

    char msg[64];
    snprintf(msg, sizeof(msg), "[Sensing] Sampling at %d Hz for %d sec ...",
             sample_rate, duration_sec, total_samples);
    Serial.print(msg);
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
    return sample_index >= total_samples;
}

void sensing_sample()
{
    if (!sample_data || sensing_is_full())
        return;

    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    uint16_t dt_ms = (uint16_t)(millis() - start_millis); // unsigned numbers are better for time deltas, as negative values are not expected

    uint32_t base = sample_index * channels;
    sample_data[base + 0] = dt_ms;
    sample_data[base + 1] = ax;
    sample_data[base + 2] = ay;
    sample_data[base + 3] = az;
    sample_index++;
}

void sensing_update()
{
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

    Serial.println("=== Sensing Metadata ===");
    Serial.print("Start Time: ");
    Serial.print(start_time.year); Serial.print("-");
    Serial.print(start_time.month); Serial.print("-");
    Serial.print(start_time.day); Serial.print(" ");
    Serial.print(start_time.hour); Serial.print(":");
    Serial.print(start_time.minute); Serial.print(":");
    Serial.print(start_time.second); Serial.print(".");
    Serial.println(start_time.millisecond);

    Serial.print("Sample Rate: ");
    Serial.print(sample_rate); Serial.println(" Hz");

    Serial.print("Duration: ");
    Serial.print((float)sample_index / sample_rate, 2); Serial.println(" sec");

    Serial.print("Total Samples: ");
    Serial.println(sample_index);

    Serial.print("Time Offset: ");
    Serial.print(start_time.offset_ms);
    Serial.println(" ms");

    Serial.println("=== Sample Data ===");

    for (uint32_t i = 0; i < sample_index; ++i)
    {
        uint32_t base = i * channels;
        uint16_t t_ms = sample_data[base + 0];
        float ax = sample_data[base + 1] / 16384.0f;
        float ay = sample_data[base + 2] / 16384.0f;
        float az = sample_data[base + 3] / 16384.0f;

        char time_str[8];
        snprintf(time_str, sizeof(time_str), "%05d", t_ms);  // fixed-width ms

        Serial.print(time_str); Serial.print(" ms | ");
        Serial.print("X: "); Serial.print(ax, 6);
        Serial.print(" Y: "); Serial.print(ay, 6);
        Serial.print(" Z: "); Serial.println(az, 6);
    }

    Serial.println("====================");

    free(sample_data);
    sample_data = nullptr;
    sample_index = 0;

    mqtt_enabled = true;
}
