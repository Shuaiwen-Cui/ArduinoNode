# WIFI

本项目使用的主控板Arduino Uno R4 Wifi板载了ESP32芯片，支持WiFi和蓝牙功能。基于Arduino官方的WiFi库WiFiS3, 我们可以轻松地连接到WiFi网络。在本项目中，wifi是节点进行物联网通信的物理基础设施。

**wifi.hpp**

```cpp
#pragma once

#include "config.hpp"

// Function declaration
void connect_to_wifi();


```

**wifi.cpp**

```cpp
#include "wifi.hpp"
#include <WiFiS3.h>

void connect_to_wifi()
{
    Serial.print("[INIT] <WIFI> Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    int status = WL_IDLE_STATUS;

    while (status != WL_CONNECTED)
    {
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(1000);
    }

    Serial.println("[INIT] <WIFI> Connected.");
    Serial.print("[INIT] <WIFI> IP address: ");
    Serial.println(WiFi.localIP());
}

```

如代码所示，只需要在主程序中合适位置调用`connect_to_wifi()`函数即可完成WiFi连接。