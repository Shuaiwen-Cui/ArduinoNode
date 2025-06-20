#include "mqtt.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>

// === MQTT Enable Flag ===
bool mqtt_enabled = true;

// === Command Flags ===
bool cmd_flg_ntp = false;
bool cmd_flg_scheduled_sensing = false;

// === Parsed Command Variables ===
char cmd_sensing_raw[128];
NodeTime parsed_start_time;
uint16_t parsed_freq = 0;
uint16_t parsed_duration = 0;

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

        // Parse CMD_SENSING_YYYY-MM-DD_HH:MM:SS_100Hz_20s
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

            char time_buf[64];
            snprintf(time_buf, sizeof(time_buf),
                     "[MQTT] Parsed Time: %04d-%02d-%02d %02d:%02d:%02d",
                     y, mo, d, h, mi, s);
            Serial.println(time_buf);

            char rate_buf[32];
            snprintf(rate_buf, sizeof(rate_buf), "[MQTT] Parsed Rate: %d Hz", rate);
            Serial.println(rate_buf);

            char dur_buf[32];
            snprintf(dur_buf, sizeof(dur_buf), "[MQTT] Parsed Duration: %d sec", dur);
            Serial.println(dur_buf);
        }
        else
        {
            Serial.println("[MQTT] Failed to parse CMD_SENSING command.");
            cmd_flg_scheduled_sensing = false;
        }
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
