# 射频通信

射频通信是指通过无线电波进行信息传输的技术。它广泛应用于无线电、电视、手机、卫星通信等领域。射频通信的基本原理是将信息调制到射频信号上，通过天线发射和接收。本项目使用nRF24L01无线模块进行射频通信。

!!! tip
    正如基于WIFI的MQTT命令通信一样，具体细节我们在命令与反馈部分会详细介绍。这里我们先介绍nRF24L01无线模块的基本通信功能。

以下是nRF24L01无线模块的相关代码实现：

**rf.hpp**

```cpp
#pragma once
#include <Arduino.h>
#include <RF24.h>
#include "config.hpp"

#define RF_CHANNEL 108
#define RF_PIPE_BASE 0xF0F0F0F000LL

struct RFMessage
{
    uint8_t from_id;
    uint8_t to_id;
    char payload[22];       
    uint64_t timestamp_ms; 
};

extern bool node_online[NUM_NODES + 1]; 

bool rf_init();
bool rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack = false);
bool rf_receive(RFMessage &msg, unsigned long timeout_ms);
bool rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries);

void rf_stop_listening();
void rf_start_listening();
void rf_set_rx_address(uint8_t id);
String rf_format_address(uint16_t node_id);

void rf_check_node_status();  // Gateway and Leaf share this
// void rf_sync_log_number();    // Gateway only

```

**rf.cpp**

```cpp
#include "rf.hpp"
#include <SPI.h>
#include "logging.hpp"

RF24 radio(9, 8);

bool node_online[NUM_NODES + 1] = {false}; // Default all to offline

String rf_format_address(uint16_t node_id)
{
    char buf[6];
    snprintf(buf, sizeof(buf), "N%03d", node_id);
    return String(buf);
}

void rf_set_rx_address(uint8_t id)
{
    uint64_t address = RF_PIPE_BASE | id;
    radio.openReadingPipe(1, address);
    radio.setAutoAck(true);
}

bool rf_init()
{
    if (!radio.begin())
    {
        Serial.println("[INIT] <RF> Initialization failed.");
        return false;
    }

    radio.setPALevel(RF24_PA_HIGH);
    radio.setDataRate(RF24_250KBPS); // Use 250kbps for better range and reliability
    radio.setChannel(RF_CHANNEL);
    radio.setRetries(5, 15);
    radio.enableDynamicPayloads();
    radio.setCRCLength(RF24_CRC_16);

    // Set RX address to this node's own ID so it can receive messages addressed to itself
    rf_set_rx_address(NODE_ID);
    radio.startListening();

    Serial.print("[INIT] <RF> Initialized. Listening on ");
    Serial.println(rf_format_address(NODE_ID));
    return true;
}

void rf_stop_listening() { radio.stopListening(); }
void rf_start_listening() { radio.startListening(); }

bool rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack)
{
    uint64_t tx_address = RF_PIPE_BASE | to_id;
    radio.openWritingPipe(tx_address);
    return radio.write(&msg, sizeof(RFMessage), require_ack);
}

bool rf_receive(RFMessage &msg, unsigned long timeout_ms)
{
    unsigned long start_time = millis();
    while (millis() - start_time < timeout_ms)
    {
        if (radio.available())
        {
            radio.read(&msg, sizeof(RFMessage));
            return true;
        }
    }
    return false;
}

bool rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries)
{
    for (uint8_t attempt = 0; attempt < retries; ++attempt)
    {
        rf_stop_listening();
        bool sent = rf_send(to_id, msg);
        rf_start_listening();

        if (!sent)
        {
            Serial.print("[RF] Send failed (attempt ");
            Serial.print(attempt + 1);
            Serial.println(")");
            continue;
        }

        RFMessage response;
        if (rf_receive(response, timeout_ms) && response.to_id == NODE_ID)
            return true;

        Serial.print("[RF] No response received (attempt ");
        Serial.print(attempt + 1);
        Serial.println(")");
    }
    return false;
}

void rf_check_node_status()
{
#ifdef GATEWAY
    delay(2000); // Allow time for radio to stabilize
    load_log_number();
    Serial.println("[RF] Checking node status and syncing LOG_NUMBER...");

    const unsigned long timeout_ms = 200;

    for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
    {
        if (node_id == NODE_ID)
            continue;

        // Prepare LOG message
        RFMessage msg;
        msg.from_id = NODE_ID;
        msg.to_id = node_id;
        snprintf(msg.payload, sizeof(msg.payload), "LOG %d", log_number);
        msg.timestamp_ms = millis();

        rf_stop_listening();
        rf_send(node_id, msg, false);
        rf_start_listening();

        Serial.print("[GATEWAY] Sent LOG_NUMBER ");
        Serial.print(log_number);
        Serial.print(" to Node ");
        Serial.println(node_id);

        // Wait for PONG reply with confirmation
        bool online = false;
        RFMessage reply;
        if (rf_receive(reply, timeout_ms) &&
            reply.to_id == NODE_ID &&
            reply.from_id == node_id &&
            strncmp(reply.payload, "PONG", 4) == 0)
        {
            int confirmed_log = 0;
            sscanf(reply.payload, "PONG %d", &confirmed_log);
            Serial.print("  - Node ");
            Serial.print(node_id);
            Serial.print(" is ONLINE. Confirmed LOG_NUMBER = ");
            Serial.println(confirmed_log);
            online = true;
        }
        else
        {
            Serial.print("  - Node ");
            Serial.print(node_id);
            Serial.println(" is OFFLINE or unresponsive.");
        }

        node_online[node_id] = online;
    }

#endif

#ifdef LEAFNODE
    Serial.println("[LEAFNODE] Waiting for LOG_NUMBER from GATEWAY...");

    while (true)
    {
        RFMessage msg;
        if (rf_receive(msg, 100))
        {
            if (strncmp(msg.payload, "LOG", 3) == 0 && msg.to_id == NODE_ID)
            {
                int received_log = 0;
                sscanf(msg.payload, "LOG %d", &received_log);
                log_number = received_log;
                save_log_number();

                Serial.print("[LEAFNODE] Received and saved LOG_NUMBER = ");
                Serial.println(log_number);

                // Respond with PONG and confirmed log number
                RFMessage reply;
                reply.from_id = NODE_ID;
                reply.to_id = msg.from_id;
                snprintf(reply.payload, sizeof(reply.payload), "PONG %d", log_number);
                reply.timestamp_ms = millis();

                rf_stop_listening();
                rf_send(msg.from_id, reply, false);
                rf_start_listening();

                Serial.println("[LEAFNODE] PONG with LOG_NUMBER sent.");
                break; // Exit after one successful exchange
            }
        }
    }
#endif
}

```

## 关键函数

| 函数名 | 功能描述 |
| --- | --- |
| `rf_init()` | 初始化 nRF24L01 模块，设置频道、数据速率等参数，并开始监听。 |
| `rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack = false)` | 向指定节点 ID 发送 RFMessage 消息，可选择是否需要确认应答。 |
| `rf_receive(RFMessage &msg, unsigned long timeout_ms)` | 在指定超时时间内接收 RFMessage 消息。 |
| `rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries)` | 发送消息后等待接收响应，支持设置重试次数。 |
| `rf_stop_listening()` | 停止监听，切换到发送模式。 |
| `rf_start_listening()` | 开始监听，切换到接收模式。 |
| `rf_set_rx_address(uint8_t id)` | 设置接收地址，指定要监听的节点 ID。 |
| `rf_format_address(uint16_t node_id)` | 将节点 ID 格式化为字符串形式，便于打印和调试。 |

---

## RFMessage 结构体说明

`RFMessage` 结构体用于定义无线通信中传输的消息格式，包含以下字段：

- `from_id`：发送方节点 ID。
- `to_id`：接收方节点 ID。
- `payload`：消息的有效载荷，最大长度为 22 字节。
- `timestamp_ms`：消息的发送时间戳，单位为毫秒。

---

## rf_init() 函数说明

`rf_init()` 函数用于初始化 nRF24L01 模块。函数首先尝试启动模块，如果失败返回 `false`。接着配置发射功率、数据速率、频道、重试机制，并启用动态有效载荷和 CRC 校验。随后调用 `rf_set_rx_address(NODE_ID)` 设置本节点接收地址，并进入监听模式。初始化成功后会打印当前节点地址并返回 `true`。

---

## rf_set_rx_address() 函数说明

`rf_set_rx_address(uint8_t id)` 函数用于设置 nRF24L01 模块的接收地址。它将节点 ID 与预设基地址 `RF_PIPE_BASE` 进行位或操作，生成完整地址，然后调用 `radio.openReadingPipe(1, address)` 打开接收管道，并启用自动应答功能。

---

## rf_format_address() 函数说明

`rf_format_address(uint16_t node_id)` 函数将节点 ID 格式化为字符串，格式为 "Nxxx"（如 N001、N100），使用 `snprintf` 实现，方便打印与调试。

---

## rf_send() 函数说明

`rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack)` 函数用于发送 RFMessage 消息到指定接收节点。它先生成对应的写入地址并执行发送。如果 `require_ack` 为 `true`，会等待接收方的确认应答。函数返回发送是否成功的布尔值。

---

## rf_receive() 函数说明

`rf_receive(RFMessage &msg, unsigned long timeout_ms)` 函数用于在给定超时时间内尝试接收一条消息。如果成功接收到数据，则将其写入传入的 `msg` 对象，并返回 `true`；否则在超时后返回 `false`。

---

## rf_send_then_receive() 函数说明

`rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries)` 函数用于发送消息并等待回应。它会尝试最多 `retries` 次发送，每次发送后切换至监听模式等待响应。如果收到的响应中 `to_id` 与当前节点 ID 匹配，则返回 `true`，否则继续重试。所有尝试失败时返回 `false`。

---

## rf_stop_listening() 和 rf_start_listening() 函数说明

- `rf_stop_listening()` 用于停止监听模式，切换至发送模式，通过调用 `radio.stopListening()` 实现。
- `rf_start_listening()` 用于启动监听模式，通过调用 `radio.startListening()` 实现接收功能。
