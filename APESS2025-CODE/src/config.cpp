#include "config.hpp"

uint64_t sensing_scheduled_start_ms = 0;
uint64_t sensing_scheduled_end_ms = 0;
uint32_t default_sensing_rate_hz = 200;
uint32_t default_sensing_duration_s = 300;
uint32_t sensing_rate_hz = default_sensing_rate_hz ;
uint32_t sensing_duration_s = default_sensing_duration_s;
uint16_t parsed_freq = 0;
uint16_t parsed_duration = 0;

float cali_scale_x = 1.0f; // Calibration scale for X-axis
float cali_scale_y = 1.0f; // Calibration scale for Y-axis
float cali_scale_z = 1.0f; // Calibration scale for Z-axis

void print_node_config()
{
  Serial.println("=== Node Configuration Info ===");

#ifdef GATEWAY
  Serial.println("Node Type     : GATEWAY");
#endif
#ifdef LEAFNODE
  Serial.println("Node Type     : LEAFNODE");
#endif

  Serial.print("Node ID       : ");
  Serial.println(NODE_ID);

  Serial.print("Total Nodes   : ");
  Serial.println(NUM_NODES);

  Serial.println("------ WiFi ------");
  Serial.print("SSID          : ");
  Serial.println(WIFI_SSID);
  Serial.print("Password      : ");
  Serial.println(WIFI_PASSWORD);

  Serial.println("------ MQTT ------");
  Serial.print("Client ID     : ");
  Serial.println(MQTT_CLIENT_ID);
  Serial.print("Broker Addr   : ");
  Serial.println(MQTT_BROKER_ADDRESS);
  Serial.print("Broker Port   : ");
  Serial.println(MQTT_BROKER_PORT);
  Serial.print("Username      : ");
  Serial.println(MQTT_USERNAME);
  Serial.print("Password      : ");
  Serial.println(MQTT_PASSWORD);
  Serial.print("Pub Topic     : ");
  Serial.println(MQTT_TOPIC_PUB);
  Serial.print("Sub Topic     : ");
  Serial.println(MQTT_TOPIC_SUB);

  Serial.println("===============================");
}
