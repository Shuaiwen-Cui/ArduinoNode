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

      sensing_scheduled_start_ms = parsed_start_time.compute_ms_from_calendar();
      SensingSchedule.unix_ms = sensing_scheduled_start_ms;
      SensingSchedule.unix_epoch = sensing_scheduled_start_ms / 1000;
      SensingSchedule.set_calendar(); // Update calendar fields based on scheduled start time
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
