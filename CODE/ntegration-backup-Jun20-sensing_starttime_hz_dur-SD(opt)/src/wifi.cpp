#include "wifi.hpp"
#include <WiFiS3.h>

void connect_to_wifi()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    int status = WL_IDLE_STATUS;

    while (status != WL_CONNECTED)
    {
        status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(1000);
    }

    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}
