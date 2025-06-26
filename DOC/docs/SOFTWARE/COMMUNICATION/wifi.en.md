# WIFI

The microcontroller used in this project is the Arduino Uno R4 Wifi, which has an onboard ESP32 chip that supports both WiFi and Bluetooth functionalities. Using the official Arduino WiFi library, WiFiS3, we can easily connect to a WiFi network. In this project, WiFi serves as the physical infrastructure for IoT communication.

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

As shown in the code, you can complete the WiFi connection by calling the `connect_to_wifi()` function at an appropriate place in your main program.