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
    如上面代码所示，mqtt部分主要是有几个关键内容，包括初始化，循环，发布消息，和回调函数。其中回调部分我们在命令与反馈部分详细介绍。
    

## MQTT初始化

如函数`mqtt_setup()`所示，MQTT初始化主要是设置服务器地址和端口，设置回调函数，并尝试连接到MQTT服务器。如果连接成功，则订阅指定的主题。相关宏已经在`config.hpp`中定义。

## MQTT循环

如函数`mqtt_loop()`所示，MQTT循环主要是检查连接状态，如果断开则重新连接,每次调用`mqtt_loop()`都会向MQTT服务器发送心跳包以保持连接活跃。

## MQTT发布测试消息

如函数`mqtt_publish_test()`所示，基于Arduino函数库，在MQTT链接状态下，调用函数`mqtt_client.publish()`可以向指定主题发布消息。这里我们发布了一个简单的测试消息，该功能是数据上传的基础。注意，每次发送消息，其容量有限制，对于大体量数据，可能需要分段发送或者使用其他方式。

## MQTT回调函数

如函数`mqtt_callback()`所示，当接收到订阅的主题消息时，会调用此回调函数。函数会解析消息内容，并根据不同的命令执行相应的操作。通常，如果对应操作比较耗时，我们会在回调函数中设置标志位或者设置状态机，以便在主循环中处理。在主循环中，根据对应状态和标志量来执行具体的操作。在这里，我们主要基于MQTT回调来实现远程控制。