# MQTT - Message Queuing Telemetry Transport Protocol

Message Queuing Telemetry Transport (MQTT) is a lightweight messaging protocol designed for low-bandwidth, high-latency, or unreliable networks. It is commonly used for communication between Internet of Things (IoT) devices. In simple terms, the MQTT server acts as an intermediary, relaying messages between users and devices. This section will cover how to connect an Arduino to an MQTT server for remote control and monitoring.

Following are the MQTT-related code files used in this project:

**mqtt.hpp**

```cpp
#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>
#include "time.hpp"
#include "sensing.hpp"
#include "rgbled.hpp"

// Create a WiFi client and wrap it in PubSubClient

extern WiFiClient wifi_client;
extern PubSubClient mqtt_client;

// === Retrieval Filename
extern char retrieval_filename[32];

// to slow down MQTT loop
bool should_run_mqtt_loop();

// Initialize and connect to MQTT broker
void mqtt_setup();

// Call this in loop() to keep MQTT alive
void mqtt_loop();

// Publish a test message to broker
void mqtt_publish_test();

```

**mqtt.cpp**
```cpp
#include "mqtt.hpp"

// Create a WiFi client and wrap it in PubSubClient
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// Parsed Command Variables
char cmd_sensing_raw[128];

// === Retrieval Filename
char retrieval_filename[32] = {0}; // Initialize as empty string

// to slow down MQTT loop
bool should_run_mqtt_loop()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last >= 500)
  {
    last = now;
    return true;
  }
  return false;
}

// Callback when subscribed message is received
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("[COMMUNICATION] <MQTT> Message received [");
  Serial.print(topic);
  Serial.print("]: ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; ++i)
  {
    message[i] = (char)payload[i];
    Serial.print(message[i]);
  }
  message[length] = '\0';
  Serial.println();

  // Clean trailing \r or \n
  while (length > 0 && (message[length - 1] == '\r' || message[length - 1] == '\n'))
  {
    message[--length] = '\0';
  }

  String msg_str(message);

  if (msg_str == "CMD_NTP")
  {
    node_status.node_flags.gateway_ntp_required = true;
    node_status.node_flags.leafnode_ntp_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_NTP received.");

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::WIFI_COMMUNICATING);
    rgbled_set_by_state(NodeState::WIFI_COMMUNICATING); // Set LED to blue during NTP sync
  }
  else if (msg_str == "CMD_GATEWAY_NTP")
  {
    node_status.node_flags.gateway_ntp_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_GATEWAY_NTP received.");

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::WIFI_COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
  }
  else if (msg_str == "CMD_LEAFNODE_NTP")
  {
    node_status.node_flags.leafnode_ntp_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_LEAFNODE_NTP received.");

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::WIFI_COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
  }
  else if (msg_str == "CMD_RF_SYNC")
  {
    node_status.node_flags.time_rf_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_RF_SYNC received.");
  }
  else if (msg_str.startsWith("CMD_SENSING_"))
  {
    strncpy(cmd_sensing_raw, message, sizeof(cmd_sensing_raw) - 1);
    cmd_sensing_raw[sizeof(cmd_sensing_raw) - 1] = '\0';
    node_status.node_flags.sensing_requested = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_SENSING received.");

    int y, mo, d, h, mi, s;
    int rate, dur;
    int matched = sscanf(message,
                         "CMD_SENSING_%d-%d-%d_%d:%d:%d_%dHz_%ds",
                         &y, &mo, &d, &h, &mi, &s, &rate, &dur);
    int ms_value = 0;
    if (matched == 8)
    {
      parsed_start_time.year = (uint16_t)y;
      parsed_start_time.month = (uint8_t)mo;
      parsed_start_time.day = (uint8_t)d;
      parsed_start_time.hour = (uint8_t)h;
      parsed_start_time.minute = (uint8_t)mi;
      parsed_start_time.second = (uint8_t)s;
      parsed_start_time.ms = ms_value;

      parsed_freq = (uint16_t)rate;
      parsed_duration = (uint16_t)dur;

      uint64_t now_unix_ms = Time.estimate_time_ms();
      if (now_unix_ms < parsed_start_time.compute_ms_from_calendar())
      {
        Serial.println("[MQTT] Sensing start time is in the future, scheduling sensing.");
        sensing_scheduled_start_ms = parsed_start_time.compute_ms_from_calendar();
        SensingSchedule.unix_ms = sensing_scheduled_start_ms;
        SensingSchedule.unix_epoch = sensing_scheduled_start_ms / 1000;
        SensingSchedule.set_calendar();                                                   // Update calendar fields based on scheduled start time
        sensing_scheduled_end_ms = sensing_scheduled_start_ms + (parsed_duration * 1000); // ms

        sensing_rate_hz = parsed_freq;
        sensing_duration_s = parsed_duration;

        node_status.node_flags.sensing_scheduled = true;

        char buf[128];
        snprintf(buf, sizeof(buf), "[MQTT] Sensing scheduled, sampling at %d Hz for %d seconds, starting at %04d-%02d-%02d %02d:%02d:%02d",
                 parsed_freq, parsed_duration,
                 parsed_start_time.year, parsed_start_time.month, parsed_start_time.day,
                 parsed_start_time.hour, parsed_start_time.minute, parsed_start_time.second);
        Serial.println(buf);
      }
      else
      {
        Serial.println("[MQTT] Sensing start time is in the past, ignoring command.");
        node_status.node_flags.sensing_requested = false;

        // feedback to the mqtt broker
        mqtt_client.publish(MQTT_TOPIC_PUB, "Sensing command ignored: start time is in the past!");

        rgbled_set_all(CRGB::Red); // Set LED to red to indicate error
        delay(3000); // Wait for 2 seconds to indicate error
        if (node_status.get_state() == NodeState::IDLE)
        {
          rgbled_set_by_state(NodeState::IDLE); // Reset LED to IDLE state
        }
      }
    }
    else
    {
      Serial.println("[MQTT] Failed to parse CMD_SENSING command.");
      node_status.node_flags.sensing_requested = false;
    }
  }
  else if (msg_str.startsWith("CMD_RETRIEVAL_"))
  {
    const char *filename_part = message + 14;
    snprintf(retrieval_filename, sizeof(retrieval_filename), "/%s.txt", filename_part);
    node_status.node_flags.data_retrieval_requested = true;
    node_status.node_flags.data_retrieval_sent = false; // Reset sent flag for new retrieval

    Serial.print("[COMMUNICATION] <CMD> CMD_RETRIEVAL received: ");
    Serial.println(retrieval_filename);

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::WIFI_COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during data retrieval
  }
  else if (msg_str == "CMD_REBOOT")
  {
    node_status.node_flags.reboot_required_gateway = true;
    node_status.node_flags.reboot_required_leafnode = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_REBOOT received.");
  }
  else if (msg_str == "CMD_GATEWAY_REBOOT")
  {
    node_status.node_flags.reboot_required_gateway = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_GATEWAY_REBOOT received.");
  }
  else if (msg_str == "CMD_LEAFNODE_REBOOT")
  {
    node_status.node_flags.reboot_required_leafnode = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_LEAFNODE_REBOOT received.");
  }
  else
  {
    Serial.println("[COMMUNICATION] <CMD> Unknown command.");
  }
}

// Connect to MQTT broker
void mqtt_setup()
{
  mqtt_client.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);
  mqtt_client.setCallback(mqtt_callback);

  Serial.println("[INIT] <MQTT> Connecting to broker... ");
  while (!mqtt_client.connected())
  {
    if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("[INIT] <MQTT> Connected.");
      mqtt_client.subscribe(MQTT_TOPIC_SUB);
      node_status.node_flags.mqtt_connected = true;
    }
    else
    {
      Serial.print("[INIT] <MQTT> Failed, Return Code = ");
      Serial.print(mqtt_client.state());
      Serial.println(" -> retrying in 2 sec...");
      delay(2000);
    }
  }
}

// Keep MQTT connection alive
void mqtt_loop()
{
  if (!mqtt_client.connected())
  {
    Serial.println("[COMMUNICATION] <MQTT> Reconnecting...");
    mqtt_setup();
  }
  mqtt_client.loop();
}

// Publish a test message
void mqtt_publish_test()
{
  if (mqtt_client.connected())
  {
    mqtt_client.publish(MQTT_TOPIC_PUB, "Hello from Arduino MQTT!");
    Serial.println("[TEST] <MQTT> Test message published.");
  }
}

```

!!! note
    As shown in the code above, the MQTT part mainly consists of several key components including initialization, loop, message publishing, and callback functions. The callback part is explained in detail in the command and feedback section.

## MQTT Setup

As shown in the function `mqtt_setup()`, MQTT initialization mainly involves setting the server address and port, setting the callback function, and attempting to connect to the MQTT server. If the connection is successful, it subscribes to the specified topic. The relevant macros are defined in `config.hpp`.

## MQTT Loop

As shown in the function `mqtt_loop()`, the MQTT loop mainly checks the connection status. If disconnected, it attempts to reconnect. Each time `mqtt_loop()` is called, it sends a heartbeat packet to the MQTT server to keep the connection alive.

## MQTT Publish Test

As shown in the function `mqtt_publish_test()`, based on the Arduino library, when the MQTT connection is established, calling the function `mqtt_client.publish()` can publish messages to the specified topic. Here we publish a simple test message, which is the basis for data upload. Note that each time a message is sent, there is a limit on its size. For large data volumes, it may need to be sent in segments or use other methods.

## MQTT Callback

As shown in the function `mqtt_callback()`, when a message is received on the subscribed topic, this callback function is invoked. The function parses the message content and executes corresponding actions based on different commands. Typically, if the corresponding operation is time-consuming, we set a flag or state machine in the callback function to handle it in the main loop. In the main loop, specific operations are executed based on the corresponding state and flags. Here, we mainly implement remote control based on MQTT callbacks.

