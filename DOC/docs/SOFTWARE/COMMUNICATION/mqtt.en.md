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
#include "mqtt_cmd.hpp"


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
char retrieval_filename[32];

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

