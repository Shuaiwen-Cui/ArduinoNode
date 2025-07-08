# MQTT - 消息队列遥测传输协议

消息队列遥测传输协议（MQTT）是一种轻量级的消息传递协议，专为低带宽、高延迟或不可靠网络环境设计。它通常用于物联网（IoT）设备之间的通信。简单来说，MQTT服务器就相当于一个中介，在用户和设备之间传递消息。本节将介绍如何将Arduino链接到MQTT服务器，从而实现远程控制和监控。

下面是本项目中使用的MQTT相关代码文件：

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
    如上面代码所示，mqtt部分主要是有几个关键内容，包括初始化，循环，发布消息，和回调函数。其中回调部分我们在命令与反馈部分详细介绍。
    

## MQTT初始化

如函数`mqtt_setup()`所示，MQTT初始化主要是设置服务器地址和端口，设置回调函数，并尝试连接到MQTT服务器。如果连接成功，则订阅指定的主题。相关宏已经在`config.hpp`中定义。

## MQTT循环

如函数`mqtt_loop()`所示，MQTT循环主要是检查连接状态，如果断开则重新连接,每次调用`mqtt_loop()`都会向MQTT服务器发送心跳包以保持连接活跃。

## MQTT发布测试消息

如函数`mqtt_publish_test()`所示，基于Arduino函数库，在MQTT链接状态下，调用函数`mqtt_client.publish()`可以向指定主题发布消息。这里我们发布了一个简单的测试消息，该功能是数据上传的基础。注意，每次发送消息，其容量有限制，对于大体量数据，可能需要分段发送或者使用其他方式。

## MQTT回调函数

如函数`mqtt_callback()`所示，当接收到订阅的主题消息时，会调用此回调函数。函数会解析消息内容，并根据不同的命令执行相应的操作。通常，如果对应操作比较耗时，我们会在回调函数中设置标志位或者设置状态机，以便在主循环中处理。在主循环中，根据对应状态和标志量来执行具体的操作。在这里，我们主要基于MQTT回调来实现远程控制。