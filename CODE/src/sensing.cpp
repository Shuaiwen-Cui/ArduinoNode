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
#include "wifi.hpp"

static File data_file;
static uint32_t last_sample_time = 0;
static uint32_t t_start_ms = 0;
static uint32_t sample_count = 0;
static char filename[32];
static char printbuffer[64];

bool sensing_prepare()
{
    sample_count = 0;
    t_start_ms = sensing_scheduled_start_ms;
    last_sample_time = t_start_ms;

    load_log_number(); // Load current log number from persistent storage
    snprintf(filename, sizeof(filename), "N%03d_%03d.txt", NODE_ID, log_number + 1);

    Serial.print("[SD] Opening file for streaming: ");
    Serial.println(filename);

    data_file = SD.open(filename, FILE_WRITE);
    if (!data_file)
    {
        Serial.println("[SD] Failed to open file.");
        return false;
    }

    data_file.println("=============== Sampling Metadata ===============");
#ifdef NODE_ID
    data_file.print("Node ID: ");
    data_file.println(NODE_ID);
#endif

    // === Convert scheduled start time ===
    CalendarTime cal = calendar_from_unix_milliseconds(sensing_scheduled_start_ms);
    snprintf(printbuffer, sizeof(printbuffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             cal.year, cal.month, cal.day, cal.hour, cal.minute, cal.second, cal.ms);

    data_file.print("Start Time: ");
    data_file.println(printbuffer);

    data_file.print("Sampling Rate: ");
    data_file.print(sensing_rate_hz);
    data_file.println(" Hz");

    data_file.print("Duration: ");
    data_file.print(sensing_duration_s);
    data_file.println(" s");

    data_file.println("================= Sampling Data =================");
    data_file.println("time_ms  , ax      , ay      , az");

    Serial.println("[SENSING] Sensing started (streaming mode).");
    return true;
}

void sensing_sample_once()
{
    // Check current time
    uint32_t now_ms = Time.get_time();

    // Check if we should sample
    if (now_ms - last_sample_time >= (1000 / sensing_rate_hz))
    {
        // if yes, update the last sample time
        last_sample_time += (1000 / sensing_rate_hz);

        // Prepare the variables for reading IMU data
        int16_t ax, ay, az;

        // Read acceleration data from the IMU, to be completed by students, refering to mpu6050.hpp and mpu6050.cpp
        imu_get_acceleration(ax, ay, az);

        // Calculate the elapsed time since the start of sensing
        uint32_t elapsed = now_ms - t_start_ms;

        // Converting raw acceleration data to g's using the scaling factor and calibration factors
        float ax_g = ax * cali_scale_x / 16384.0f;
        float ay_g = ay * cali_scale_y / 16384.0f;
        float az_g = az * cali_scale_z / 16384.0f;

        // print the data to the SD card file
        char line[64];
        snprintf(line, sizeof(line), "%8lu,%8.6f,%8.6f,%8.6f", elapsed, ax_g, ay_g, az_g);
        data_file.println(line);

        // Update the number of samples taken
        sample_count++;
    }
}

void sensing_stop()
{
    Serial.print("[SENSING] Sampling completed. ");
    Serial.print(sample_count);
    Serial.println(" samples collected.");

    if (data_file)
    {
        data_file.close();
        Serial.print("[SD] File saved: ");
        Serial.println(filename);

        log_number++;
        save_log_number();
    }

#ifdef DATA_PRINTOUT
    // Reopen and print file content
    File f = SD.open(filename, FILE_READ);
    if (f)
    {
        Serial.println("[SD] Dumping file content:");
        while (f.available())
        {
            Serial.write(f.read());
        }
        f.close();
    }
    else
    {
        Serial.println("[SD] Failed to reopen file for reading.");
    }
#endif

#ifdef GATEWAY // this part will make the led switch on the gateway node slightly slower than the leafnodes, don't worry about it
    // Check WiFi connection, reconnect if disconnected
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[MQTT] WiFi disconnected. Reconnecting...");
        connect_to_wifi(); // Reconnect to WiFi
    }

    mqtt_loop(); // Keep MQTT connection alive
    // Publish the file name to MQTT broker
    String msg = "Sensing completed!";
    mqtt_client.publish(MQTT_TOPIC_PUB, msg.c_str());
#endif

    sample_count = 0;
}

void sensing_retrieve_file()
{
    File file = SD.open(retrieval_filename, FILE_READ);
    if (!file)
    {
        Serial.print("[Error] File not found: ");
        Serial.println(retrieval_filename);
        return;
    }

    Serial.print("[Retrieval] Reading file: ");
    Serial.println(retrieval_filename);

    size_t total_size = file.size();
    size_t bytes_sent = 0;
    size_t chunk_size = 850;
    size_t chunk_index = 1;
    size_t chunk_total = (total_size + chunk_size - 1) / chunk_size;

    char prefix[32];
    snprintf(prefix, sizeof(prefix), "%s", retrieval_filename + 1); // Remove leading '/'
    char topic[64];

    while (file.available())
    {
        char buffer[851]; // chunk_size + 1 for null terminator
        size_t len = file.readBytes(buffer, chunk_size);
        buffer[len] = '\0';

        snprintf(topic, sizeof(topic), "%s[%d/%d]:", prefix, chunk_index, chunk_total);
        String payload = String(topic) + String(buffer);

        bool ok = mqtt_client.publish(MQTT_TOPIC_PUB, payload.c_str());
        if (ok)
        {
            bytes_sent += len;
            Serial.print("[MQTT] Sent chunk ");
            Serial.print(chunk_index);
            Serial.print(" / ");
            Serial.print(chunk_total);
            Serial.print(" (");
            Serial.print(bytes_sent);
            Serial.print(" / ");
            Serial.print(total_size);
            Serial.println(" bytes)");
        }
        else
        {
            Serial.print("[Error] Failed to send chunk ");
            Serial.println(chunk_index);
        }

        chunk_index++;

        mqtt_loop(); // keep MQTT alive
        delay(50);   // throttle
    }

    file.close();

    String done_msg = String(prefix) + "[done]";
    mqtt_client.publish(MQTT_TOPIC_PUB, done_msg.c_str());
    Serial.println("[MQTT] File upload completed.");

    node_status.node_flags.data_retrieval_requested = false;
    node_status.node_flags.data_retrieval_sent = true;
}
