#include "mqtt.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>

// === MQTT Enable Flag ===
bool mqtt_enabled = true;

// === Command Flags ===
bool cmd_flg_ntp = false;
bool cmd_flg_scheduled_sensing = false;
bool cmd_flg_retrieval = false;  // 添加文件检索标志

// === Parsed Command Variables ===
char cmd_sensing_raw[128];
NodeTime parsed_start_time;
uint16_t parsed_freq = 0;
uint16_t parsed_duration = 0;

// === Retrieval Filename
char retrieval_filename[32] = {0};  // 初始化为空字符串

// === MQTT Setup ===
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message received [");
    Serial.print(topic);
    Serial.print("]: ");

    char message[length + 1];
    for (unsigned int i = 0; i < length; i++)
    {
        message[i] = (char)payload[i];
        Serial.print((char)payload[i]);
    }
    message[length] = '\0';
    Serial.println();

    if (strcmp(message, "CMD_NTP") == 0)
    {
        cmd_flg_ntp = true;
        Serial.println("[Command] CMD_NTP received.");
        mqtt_enabled = false;
    }
    else if (strncmp(message, "CMD_SENSING_", 12) == 0)
    {
        strncpy(cmd_sensing_raw, message, sizeof(cmd_sensing_raw) - 1);
        cmd_sensing_raw[sizeof(cmd_sensing_raw) - 1] = '\0';
        cmd_flg_scheduled_sensing = true;
        Serial.println("[Command] CMD_SENSING received.");

        int y, mo, d, h, mi, s;
        int rate, dur;
        int matched = sscanf(message,
                             "CMD_SENSING_%d-%d-%d_%d:%d:%d_%dHz_%ds",
                             &y, &mo, &d, &h, &mi, &s, &rate, &dur);

        if (matched == 8)
        {
            parsed_start_time = {
                .year = (uint16_t)y,
                .month = (uint8_t)mo,
                .day = (uint8_t)d,
                .hour = (uint8_t)h,
                .minute = (uint8_t)mi,
                .second = (uint8_t)s,
                .millisecond = 0,
                .offset_ms = 0};

            parsed_freq = (uint16_t)rate;
            parsed_duration = (uint16_t)dur;

            char buf[128];
            snprintf(buf, sizeof(buf), "[MQTT] Parsed Time: %04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
            Serial.println(buf);
            snprintf(buf, sizeof(buf), "[MQTT] Parsed Rate: %d Hz", rate);
            Serial.println(buf);
            snprintf(buf, sizeof(buf), "[MQTT] Parsed Duration: %d sec", dur);
            Serial.println(buf);
        }
        else
        {
            Serial.println("[MQTT] Failed to parse CMD_SENSING command.");
            cmd_flg_scheduled_sensing = false;
        }
    }
    else if (strncmp(message, "CMD_RETRIEVAL_", 14) == 0)
    {
        const char *filename_part = message + 14;
        snprintf(retrieval_filename, sizeof(retrieval_filename), "/%s.txt", filename_part);
        cmd_flg_retrieval = true;

        Serial.print("[Command] CMD_RETRIEVAL received: ");
        Serial.println(retrieval_filename);
    }
    else
    {
        Serial.println("[Command] Unknown command.");
    }
}

void mqtt_setup()
{
    mqtt_client.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);
    mqtt_client.setCallback(mqtt_callback);

    while (!mqtt_client.connected())
    {
        Serial.print("Connecting to MQTT broker... ");
        if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
        {
            Serial.println("connected.");
            mqtt_client.subscribe(MQTT_TOPIC_SUB);
        }
        else
        {
            int rc = mqtt_client.state();
            Serial.print("failed, return code = ");
            Serial.println(rc);
            delay(2000);
        }
    }
}

void mqtt_publish_test()
{
    if (mqtt_client.connected())
    {
        mqtt_client.publish(MQTT_TOPIC_PUB, "Hello from Arduino MQTT!");
    }
}

void mqtt_loop()
{
    if (mqtt_enabled)
    {
        if (!mqtt_client.connected())
        {
            mqtt_setup();
        }
        mqtt_client.loop();
    }
}
