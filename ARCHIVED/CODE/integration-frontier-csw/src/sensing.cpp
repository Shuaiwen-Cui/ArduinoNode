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

uint64_t sensing_scheduled_start_ms = 0;
uint64_t sensing_scheduled_end_ms = 0;
uint32_t sensing_rate_hz = 0;
uint32_t sensing_duration_s = 0;

SamplePoint *sample_buffer = nullptr;
uint32_t total_samples = 0;
uint32_t sample_count = 0;
uint32_t last_sample_time = 0;
uint32_t t_start_ms = 0;

bool sensing_start()
{
    total_samples = sensing_rate_hz * sensing_duration_s;
    sample_count = 0;

    sample_buffer = (SamplePoint *)malloc(total_samples * sizeof(SamplePoint));
    if (!sample_buffer)
    {
        Serial.println("[SENSING] Memory allocation failed.");
        return false;
    }

    t_start_ms = millis();
    last_sample_time = t_start_ms;
    Serial.println("[SENSING] Sensing started.");
    return true;
}

void sensing_sample_once()
{
    if (sample_count >= total_samples) return;

    uint32_t now_ms = millis();

    if (now_ms - last_sample_time >= (1000 / sensing_rate_hz))
    {
        last_sample_time += (1000 / sensing_rate_hz);

        int16_t ax, ay, az;
        imu_get_acceleration(ax, ay, az);

        uint16_t elapsed = (uint16_t)(now_ms - t_start_ms);

        sample_buffer[sample_count++] = {elapsed, ax, ay, az};
    }
}

void sensing_stop()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "[SENSING] Sampling completed. %lu samples collected.", sample_count);
    Serial.println(buf);

    // === Metadata Header ===
    Serial.println();
    Serial.println("=============== Sampling Metadata ===============");
#ifdef NODE_ID
    Serial.print("Node ID: ");
    Serial.println(NODE_ID);
#endif
    Serial.print("Start Time: ");
    Serial.println(SensingSchedule.to_string());

    Serial.print("Sampling Rate: ");
    Serial.print(sensing_rate_hz);
    Serial.println(" Hz");

    Serial.print("Duration: ");
    Serial.print(sensing_duration_s);
    Serial.println(" s");

    Serial.print("Total Samples: ");
    Serial.println(sample_count);

    Serial.println("================= Sampling Data =================");
    Serial.println(" time(ms)     ax(g)        ay(g)        az(g)");
    Serial.println("-------------------------------------------------");

    for (uint32_t i = 0; i < sample_count; ++i)
    {
        const SamplePoint &pt = sample_buffer[i];

        float ax_g = pt.ax / 16384.0f;
        float ay_g = pt.ay / 16384.0f;
        float az_g = pt.az / 16384.0f;

        char line[80];
        snprintf(line, sizeof(line),
                 "%05u       %+1.6f   %+1.6f   %+1.6f",
                 pt.elapsed_ms, ax_g, ay_g, az_g);
        Serial.println(line);
    }

    // === Save to SD Card ===
    char filename[32];
    load_log_number(); // Load current log number from persistent storage
    snprintf(filename, sizeof(filename), "N%03d_%03d.txt", NODE_ID, log_number + 1);

    Serial.print("[SD] Saving to file: ");
    Serial.println(filename);

    File file = SD.open(filename, FILE_WRITE);
    if (file)
    {
        file.println("=============== Sampling Metadata ===============");
#ifdef NODE_ID
        file.print("Node ID: ");
        file.println(NODE_ID);
#endif
        file.print("Start Time: ");
        file.println(SensingSchedule.to_string());

        file.print("Sampling Rate: ");
        file.print(sensing_rate_hz);
        file.println(" Hz");

        file.print("Duration: ");
        file.print(sensing_duration_s);
        file.println(" s");

        file.print("Total Samples: ");
        file.println(sample_count);

        file.println("================= Sampling Data =================");
        file.println("time_ms,ax,ay,az");

        for (uint32_t i = 0; i < sample_count; ++i)
        {
            const SamplePoint &pt = sample_buffer[i];
            float ax_g = pt.ax / 16384.0f;
            float ay_g = pt.ay / 16384.0f;
            float az_g = pt.az / 16384.0f;

            char data_line[64];
            snprintf(data_line, sizeof(data_line), "%u,%.6f,%.6f,%.6f", pt.elapsed_ms, ax_g, ay_g, az_g);
            file.println(data_line);
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

    // === Cleanup ===
    free(sample_buffer);
    sample_buffer = nullptr;
    sample_count = 0;
    total_samples = 0;
}

// data retrieval
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

        Serial.print("[Debug] Payload length: ");
        Serial.println(payload.length());

        bool ok = mqtt_client.publish(MQTT_TOPIC_PUB, payload.c_str());
        if (!ok)
        {
            Serial.print("[Error] Failed to send chunk ");
            Serial.println(chunk_index);
        }

        bytes_sent += len;
        chunk_index++;

        Serial.print("[Retrieval] Sent ");
        Serial.print(bytes_sent);
        Serial.print(" / ");
        Serial.print(total_size);
        Serial.println(" bytes");

        mqtt_loop(); // keep MQTT alive
        delay(50);   // throttle transmission
    }

    file.close();

    // Final marker
    String done_msg = String(prefix) + "[done]";
    mqtt_client.publish(MQTT_TOPIC_PUB, done_msg.c_str());
    Serial.println("[MQTT] File upload completed.");

    node_status.node_flags.data_retrieval_requested = false; // Reset retrieval request flag
    node_status.node_flags.data_retrieval_sent = true;

}