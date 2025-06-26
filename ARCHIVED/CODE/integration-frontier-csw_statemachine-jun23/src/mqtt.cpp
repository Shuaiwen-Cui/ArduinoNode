#include "mqtt.hpp"
#include "config.hpp"
#include "nodestate.hpp"
#include <WiFiS3.h>
#include <PubSubClient.h>

// Create a WiFi client and wrap it in PubSubClient
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// to slow down MQTT loop
bool should_run_mqtt_loop()
{
  static unsigned long last = 0;
  unsigned long now = millis();
  if (now - last >= 5000)
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

  // TODO: Handle specific commands from server
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
  if(!mqtt_client.connected())
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
