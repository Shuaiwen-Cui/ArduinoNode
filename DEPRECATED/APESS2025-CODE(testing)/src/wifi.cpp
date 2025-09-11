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
