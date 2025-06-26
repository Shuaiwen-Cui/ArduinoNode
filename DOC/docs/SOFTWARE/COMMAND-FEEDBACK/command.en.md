# COMMAND

Controlling nodes is a crucial part of sensor node development. In traditional wireless sensor networks, this is often done by controlling the gateway node, which then communicates with other nodes wirelessly. For IoT nodes, we can leverage the internet for remote control. In this project, we achieve node control over the internet based on the MQTT callback mechanism. Let's first look at the code:

```cpp
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
    node_status.set_state(NodeState::COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
  }
  else if (msg_str == "CMD_GATEWAY_NTP")
  {
    node_status.node_flags.gateway_ntp_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_GATEWAY_NTP received.");

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
  }
  else if (msg_str == "CMD_LEAFNODE_NTP")
  {
    node_status.node_flags.leafnode_ntp_required = true;
    Serial.println("[COMMUNICATION] <CMD> CMD_LEAFNODE_NTP received.");

    // switch to COMMUNICATING state
    node_status.set_state(NodeState::COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
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
    node_status.set_state(NodeState::COMMUNICATING);
    rgbled_set_all(CRGB::Blue); // Set LED to blue during data retrieval
  }
  else
  {
    Serial.println("[COMMUNICATION] <CMD> Unknown command.");
  }
}

```

MQTT's callback mechanism allows a node to respond to messages received under subscribed topics. In practice, we match the received message content against a set of predefined commands and perform corresponding actions based on the match.

Currently, we define three types of commands:

1. **CMD_NTP**: Used to request NTP time synchronization.

2. **CMD_SENSING_**: Used to initiate a sensor data acquisition task. The format is:
   `CMD_SENSING_YYYY-MM-DD_HH:MM:SS_RATE_HZ_DURATION_S`, where `YYYY-MM-DD_HH:MM:SS` specifies the start time of acquisition, `RATE_HZ` is the sampling frequency, and `DURATION_S` is the acquisition duration.

3. **CMD_RETRIEVAL_**: Used to request data retrieval, with the format `CMD_RETRIEVAL_filename`, where `filename` is the name of the file to retrieve.

Upon receiving a command, the node handles it accordingly and updates flags, variables, and the internal state machine as necessary. 

- For NTP-related commands, the node sets the corresponding flags for time synchronization and changes the LED color to blue to indicate the operation is in progress.
- For sensor acquisition commands, the node parses the parameters and sets related flags and timing variables.
- For data retrieval commands, the node sets the target file name and updates the communication state to begin retrieval.

This approach enables reliable and modular remote control of distributed sensor nodes through MQTT.
