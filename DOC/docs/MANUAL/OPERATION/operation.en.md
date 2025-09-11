# Sensor Node Operation Guide

## 1 PREPARATION

### 1.1 MQTT Command Sender

#### (1) Computer:

It is recommended to install MQTTX for publishing MQTT commands. [Download Link](https://mqttx.app/downloads)

After installation, first establish a connection with the server as follows:

![](client.png)

Then subscribe to the topic "ArduinoNode/node"

![](connection.png)

Once completed, you will start receiving messages published by various nodes.

iOS users are recommended to use MyMQTT.

### 1.2 Sensor Network

- It is recommended to charge the batteries before use.

- Ensure all nodes are securely connected and the SD card is properly inserted.

- It is advisable to power on the leaf nodes first, followed by the gateway node.

## 2 DEPLOY THE SENSOR NETWORK

Deploy the sensor nodes in the areas that need monitoring, ideally ensuring that the distance between leaf nodes and the gateway does not exceed 25 meters.

## 3 ISSUE COLLECTION COMMANDS

![](command.png)

- First, ensure that the connection to the server is active.

- Enter the topic for publishing commands: "ArduinoNode/server"

- Input the command content according to your sampling needs and the command descriptions.
  
## Appendix - ArduinoNode Command Set

### A. Command Rules

- All commands are strings sent via **MQTT** or **RF** channels.

- Time-related commands must ensure that the scheduled time is later than the current time, with at least **60 seconds** reserved for time synchronization (otherwise, the command will be rejected).

- Sampling frequency, duration, and other parameters must be integers, with units in **Hz** and **seconds**, respectively.

- File retrieval commands must specify the filename (excluding the extension).

### B. Command List and Examples

#### Basic Commands
| Command               | Description                          | Example |
|-----------------------|--------------------------------------|---------|
| `CMD_REBOOT`          | Reboot both gateway and leaf nodes   | `CMD_REBOOT` |
| `CMD_GATEWAY_REBOOT`  | Reboot gateway only                  | `CMD_GATEWAY_REBOOT` |
| `CMD_LEAFNODE_REBOOT` | Reboot leaf nodes only               | `CMD_LEAFNODE_REBOOT` |
| `CMD_RF_SYNC`         | RF time synchronization              | `CMD_RF_SYNC` |
| `CMD_NTP`             | NTP time synchronization             | `CMD_NTP` |
| `CMD_SN`              | Schedule sensing with default params | `CMD_SN` |


#### Scheduled / Fixed-Time Sensing Commands

- **Scheduled Sensing** (delay in seconds, sampling frequency, duration)
  Format: `CMD_SFN_delay_frequencyHz_durationS`
  Example: CMD_SFN_120_10Hz_60s

- **Fixed-Time Sensing** (specific time, sampling frequency, duration)
  Format: `CMD_SENSING_YYYY-MM-DD_HH:MM:SS_frequencyHz_durationS`
  Example: CMD_SENSING_2025-09-11_14:30:00_10Hz_60s

#### Data Retrieval Command

- **Data Retrieval (file name without extension)**
  Format: `CMD_RETRIEVAL_filename`
  Example: CMD_RETRIEVAL_20250911_143000

### C. Errors & Feedback
- If the command format is incorrect, time is invalid, or parameters are unreasonable, you will receive error feedback.
- The device will also indicate error status via **red LED**.

### D. Additional Notes
For extended commands or detailed parameter instructions, please refer to the source code or contact the developer.

!!! warning
    Please ensure command format and parameters are correct, otherwise the command will be rejected. For example, the maximum sampling frequency is 250Hz, recommended value is 100Hz, which is sufficient for most civil vibration monitoring needs.

!!! warning
    Data retrieval functionality is currently limited. It is recommended to obtain data directly from the SD card of each node.

!!! tip
    The most commonly used command is `CMD_SN`, which triggers sensing with **default parameters**. These defaults can be modified in the source code. Currently, the default sampling frequency is **200 Hz** and the duration is **300 seconds**.  
    Another frequently used command is the scheduled sensing command `CMD_SFN_Delay_FrequencyHz_DurationSec`. For example: CMD_SFN_120_100Hz_300s means sensing will start **120 seconds later**, with a sampling frequency of **100 Hz** and a duration of **300 seconds**.

## Appendix - LED Status Indicators

| Status               | Color     | Description   |
|----------------------|-----------|---------------|
| BOOT                 | White     | Booting/Self-test |
| IDLE                 | Green     | Idle/Standby  |
| PREPARING            | Yellow    | Preparing for sampling |
| SAMPLING             | Purple    | Sampling in progress |
| RF_COMMUNICATING     | Cyan      | RF communication |
| WIFI_COMMUNICATING   | Blue      | WiFi communication |
| ERROR                | Red       | Error/Alarm   |
| Other/Unknown        | Off       | Undefined/Shutdown |
