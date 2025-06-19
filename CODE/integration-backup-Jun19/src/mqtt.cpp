#include "mqtt.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>

// === MQTT Enabling Flag ===
bool mqtt_enabled = true;  // Allow external control

// Create a WiFi client and wrap it in PubSubClient
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// Flag definitions
bool flag_command1 = false;
bool flag_command2 = false;

// Callback function
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

    // Set flags based on command
    if (strcmp(message, "COMMAND1") == 0)
    {
        flag_command1 = true;
        Serial.println("[Command] COMMAND1 received.");
        mqtt_enabled = false;  // Disable MQTT after receiving COMMAND1
    }
    else if (strcmp(message, "COMMAND2") == 0)
    {
        flag_command2 = true;
        Serial.println("[Command] COMMAND2 received.");
    }
    else
    {
        Serial.println("[Command] Unknown command.");
    }
}

// Connect to the MQTT broker
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
            mqtt_client.subscribe(MQTT_TOPIC_SUB); // optional
        }
        else
        {
            int rc = mqtt_client.state();
            Serial.print("failed, return code = ");
            Serial.print(rc);
            Serial.print(" (");

            // Explain the return code
            switch (rc)
            {
                case -4: Serial.print("MQTT_CONNECTION_TIMEOUT"); break;
                case -3: Serial.print("MQTT_CONNECTION_LOST"); break;
                case -2: Serial.print("MQTT_CONNECT_FAILED"); break;
                case -1: Serial.print("MQTT_DISCONNECTED"); break;
                case  1: Serial.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
                case  2: Serial.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
                case  3: Serial.print("MQTT_CONNECT_UNAVAILABLE"); break;
                case  4: Serial.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
                case  5: Serial.print("MQTT_CONNECT_UNAUTHORIZED"); break;
                default: Serial.print("UNKNOWN_ERROR"); break;
            }

            Serial.println("). Retrying in 2 seconds...");
            delay(2000);
        }
    }
}

// Publish a test message
void mqtt_publish_test()
{
    if (mqtt_client.connected())
    {
        mqtt_client.publish(MQTT_TOPIC_PUB, "Hello from Arduino MQTT!");
    }
}

// Call this in the main loop to keep MQTT connection alive

void mqtt_loop()
{
    if(mqtt_enabled)
    {
        if (!mqtt_client.connected())
        {
            mqtt_setup();  // try reconnect
        }
        mqtt_client.loop();
    }
}