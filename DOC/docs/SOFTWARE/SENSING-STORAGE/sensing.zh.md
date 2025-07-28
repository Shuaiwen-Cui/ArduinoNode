# 加速度传感

采样可以说是本项目最重要的功能之一。它允许我们收集和存储来自传感器的数据，以便后续分析和处理。由于Arduino性能非常有限，本项目采用一边采样一边存储的方式来实现数据采集。由于没有引入实时操作系统，存储的过程会对采样造成一定的影响，所以无法实现很高的采样频率，但是由于本项目的展示和教学性质，采样频率不需要很高。经过测试200Hz的采样频率完全可以实现，而且由于是一边采样一边存储，所以数据上限基本上等同于SD卡的容量。

**sensing.hpp**

```cpp
#pragma once

#include <stdint.h>

#define SENSING_PREPARING_DUR_MS 5000  // Duration for preparing sensing in milliseconds

typedef struct {
    uint16_t elapsed_ms;  // Elapsed time since sensing started (ms)
    int16_t ax;
    int16_t ay;
    int16_t az;
} SamplePoint;

bool sensing_prepare();                     // Called once at the beginning of PREPARING state
void sensing_sample_once();                 // Called repeatedly during SAMPLING state
void sensing_stop();                        // Called once at the end of SAMPLING state

void sensing_retrieve_file();               // Retrieve file from SD card

```

**sensing.cpp**

```cpp
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
    uint32_t now_ms = Time.get_time();
    if (now_ms - last_sample_time >= (1000 / sensing_rate_hz))
    {
        last_sample_time += (1000 / sensing_rate_hz);

        int16_t ax, ay, az;
        imu_get_acceleration(ax, ay, az);

        uint32_t elapsed = now_ms - t_start_ms;

        float ax_g = ax * cali_scale_x / 16384.0f;
        float ay_g = ay * cali_scale_y / 16384.0f;
        float az_g = az * cali_scale_z / 16384.0f;

        char line[64];

        snprintf(line, sizeof(line), "%8lu,%8.6f,%8.6f,%8.6f", elapsed, ax_g, ay_g, az_g);
        data_file.println(line);

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


```

如上面代码所示，采样过程分为几个阶段：

1. 采样开始时间和结束时间：这一部分是在MQTT命令回调时就已经完成。

2. calling `sensing_prepare()`：在采样状态预备状态时调用，打开SD卡文件并写入采样元数据。在这个函数中调用了一个`load_log_number()`函数来加载当前日志编号，并在文件名中使用它。文件名格式为`N001_001.txt`，其中`N001`是节点ID，`001`是日志编号。在SD卡中有个文件记录了当前日志编号，采样完成后会自动增加。

3. calling `sensing_sample_once()`：在采样状态中重复调用，读取传感器数据并写入SD卡文件。每次采样都会检查是否达到了设定的采样频率（`sensing_rate_hz`），如果达到了，就读取传感器数据并写入文件。在主程序的loop中，当当前时间减去上次采样时间大于等于`1000 / sensing_rate_hz`时，就进行一次采样。采样数据包括时间戳和加速度传感器的三个轴向数据（ax, ay, az），并将其写入SD卡文件。

4. calling `sensing_stop()`：在采样状态结束时调用，关闭SD卡文件并打印采样结果。这个函数会打印采样的总数，并将文件内容重新打开以便打印到串口。

!!! info
    本项目中，由于串口输出速度很慢，会拖累采样和存储，所以在采样过程中，我们只做存储而不做串口输出。采样完成后可以重新打开文件并打印内容到串口。