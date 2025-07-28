# RADIO FREQUENCY COMMUNICATION

RF communication refers to the technology of transmitting information through radio waves. It is widely used in radio, television, mobile phones, satellite communications and other fields. The basic principle of RF communication is to modulate information onto RF signals and transmit and receive them through antennas. This project uses nRF24L01 wireless module for RF communication.

!!! tip
    Just like the MQTT command communication based on WiFi, we will introduce the specific details in the command and feedback section. Here, we will first introduce the basic communication functions of the nRF24L01 wireless module.

The following is the relevant code implementation for the nRF24L01 wireless module:

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

## Key Functions

| Function Name | Description |
| --- | --- |
| `rf_init()` | Initializes the nRF24L01 module, sets the channel, data rate, and other parameters, and begins listening. |
| `rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack = false)` | Sends an RFMessage to the specified node ID, with optional acknowledgment requirement. |
| `rf_receive(RFMessage &msg, unsigned long timeout_ms)` | Receives an RFMessage until a timeout occurs or a message is received. |
| `rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries)` | Sends a message and waits for a response, with a configurable retry mechanism. |
| `rf_stop_listening()` | Stops listening and switches to transmission mode. |
| `rf_start_listening()` | Starts listening and switches to receiving mode. |
| `rf_set_rx_address(uint8_t id)` | Sets the receiving address to listen to messages from a specific node ID. |
| `rf_format_address(uint16_t node_id)` | Formats the node ID into a human-readable string for logging/debugging. |

---

## RFMessage Structure

The `RFMessage` structure defines the format of messages exchanged over RF communication. It includes the following fields:

- `from_id`: The sender's node ID.
- `to_id`: The receiver's node ID.
- `payload`: The actual data payload of the message (max 22 bytes).
- `timestamp_ms`: The timestamp (in milliseconds) when the message is sent.

---

## rf_init() Function

The `rf_init()` function initializes the nRF24L01 module. It attempts to start the RF module and returns `false` if initialization fails. If successful, it sets transmission power level, data rate, channel, retry count, and enables features such as dynamic payloads and CRC checking. It then calls `rf_set_rx_address(NODE_ID)` to set the current node’s receiving address and starts listening. If successful, the function prints the current node’s address and returns `true`.

---

## rf_set_rx_address() Function

The `rf_set_rx_address(uint8_t id)` function sets the receiving address for the nRF24L01 module. It combines the input node ID with a predefined base address (`RF_PIPE_BASE`) to form a full address. It then calls `radio.openReadingPipe(1, address)` to open a reading pipe and enables auto-acknowledgment.

---

## rf_format_address() Function

The `rf_format_address(uint16_t node_id)` function formats a node ID into a readable string using `snprintf`, producing a string in the format "Nxxx" (e.g., N001, N100). Useful for printing and debugging.

---

## rf_send() Function

The `rf_send(uint8_t to_id, const RFMessage &msg, bool require_ack)` function sends an RFMessage to the specified `to_id`. It generates the target write pipe address and transmits the message to that address. It returns `true` on success, `false` on failure. If `require_ack` is true, the function waits for an acknowledgment from the receiver.

---

## rf_receive() Function

The `rf_receive(RFMessage &msg, unsigned long timeout_ms)` function attempts to receive an RFMessage within the given timeout. It continuously checks if data is available and reads the message into `msg` upon success. Returns `true` if a message is received before timeout, otherwise returns `false`.

---

## rf_send_then_receive() Function

The `rf_send_then_receive(const RFMessage &msg, uint8_t to_id, unsigned long timeout_ms, uint8_t retries)` function sends a message and waits for a response. It attempts to send the message up to `retries` times. After each send, it switches to listening mode and waits for a valid response. If a response is received and its `to_id` matches the current node’s ID, it returns `true`. Returns `false` if all attempts fail.

---

## rf_stop_listening() and rf_start_listening() Functions

- `rf_stop_listening()` stops the nRF24L01 from listening mode and switches to transmit mode using `radio.stopListening()`.

- `rf_start_listening()` starts listening mode using `radio.startListening()` to receive incoming messages.

