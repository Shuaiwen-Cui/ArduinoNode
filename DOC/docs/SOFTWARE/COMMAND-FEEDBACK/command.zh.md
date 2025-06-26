# 命令

如何控制节点是传感器开发中非常重要的一部分。传统的无线传感器网络可以通过控制主节点，进而通过无线通信控制其他节点。IoT节点，我们可以通过互联网进行远程控制。本项目中，我们通过互联网来实现节点控制，其基础是MQTT的回调机制。我们先来看代码：

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

MQTT的回调机制使得节点可以在收到对应订阅主题下的信息时，进行相应的处理。这里我们实际上做的是对接收到的信息和预定义的命令进行匹配，并根据匹配结果执行相应的操作。当前，我们定义了三类命令：

1. **CMD_NTP**：用于请求NTP时间同步。

2. **CMD_SENSING_**：用于请求传感器数据采集，格式为
`CMD_SENSING_YYYY-MM-DD_HH:MM:SS_RATE_HZ_DURATION_S`，其中`YYYY-MM-DD_HH:MM:SS`表示采集开始时间，`RATE_HZ`表示采样频率，`DURATION_S`表示采集持续时间。

3. **CMD_RETRIEVAL_**：用于请求数据检索，格式为`CMD_RETRIEVAL_filename`，其中`filename`是要检索的数据文件名。

在接受到命令后，节点会根据命令类型进行相应的处理，并在必要时更新标志量，变量和状态机。对于NTP命令，节点会设置NTP同步所需的标志量，并将LED灯设置为蓝色以表示正在进行NTP同步。对于传感器数据采集命令，节点会解析采集参数并设置相应的标志量和变量。对于数据检索命令，节点会设置检索文件名并更新状态。