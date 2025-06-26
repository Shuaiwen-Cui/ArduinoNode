# TIME SYNCHRONIZATION

Time synchronization is critical for wireless network devices. It ensures time consistency between devices so that timestamps, logging, and other time-related functions of data packets can work correctly. In this project, time synchronization is divided into two parts: the first part is time synchronization with the Internet, and the second part is time synchronization between devices.

The related codes are shown below:

**time.hpp**

```cpp
#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"

/*
 * Time synchronization header
 * 
 * Provides:
 * - NTP synchronization function
 * - RF-based online check and RTT estimation
 * - Stores RF latency (RTT/2) and online status for each node
 */

/* === RF Time Synchronization Configuration === */
#define RF_PING_PAYLOAD "PING"
#define RF_PONG_PAYLOAD "PONG"
#define RF_TIME_SYNC_HEADER "TIME_SYNC"
#define RF_TIME_ACK_HEADER "TIME_ACK"
#define RF_RESPONSE_WAIT_MS      300   // 300 ms wait per reply
#define RF_RETRY_PER_CYCLE         5   // Retry 5 times per send

extern int32_t node_rf_latency[NUM_NODES + 1];
extern bool node_online[NUM_NODES + 1];

bool sync_time_ntp();
bool sync_check_rf_online();
bool rf_time_sync();
```

**time.cpp**

```cpp
#include "timesync.hpp"
#include "config.hpp"
#include "time.hpp"
#include "rf.hpp"
#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 28800, 60000);

int32_t node_rf_latency[NUM_NODES + 1] = {0};
bool node_online[NUM_NODES + 1] = {false};

bool sync_time_ntp()
{
    timeClient.begin();
    if (!timeClient.update())
    {
        Serial.println("[COMMUNICATION] <NTP> Failed to get NTP time.");
        return false;
    }

    uint64_t epoch = timeClient.getEpochTime();
    uint16_t current_millis = millis();

    Time.set_time_epoch(epoch);
    Time.last_update_epoch = epoch;
    Time.last_update_ms = current_millis % 1000 + (epoch * 1000); // Convert to ms, keeping current millis for ms part
    Time.mcu_base_ms = current_millis;
    Time.mcu_time_ms = current_millis;
    Time.delta_ms = 0;

    Serial.print("[COMMUNICATION] <NTP> Synchronized UNIX epoch: ");
    Serial.println(epoch);
    Serial.print("[COMMUNICATION] <NTP> Current time: ");
    Time.print();
    return true;
}

bool sync_check_rf_online()
{
#ifdef GATEWAY
    Serial.println("[COMMUNICATION] <SYNC> Master checking nodes...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        bool success = false;
        uint32_t rtt = 0;

        for (uint8_t attempt = 0; attempt < 3; ++attempt)
        {
            // Construct PING message
            RFMessage msg;
            msg.from_id = NODE_ID;
            msg.to_id = id;
            strncpy(msg.payload, RF_PING_PAYLOAD, sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0';

            // Stop listening before sending
            rf_stop_listening();
            bool sent = rf_send(id, msg, false); // No ACK requested
            rf_start_listening();

            // If send failed, retry
            if (!sent)
            {
                Serial.print(" - Node ");
                Serial.print(id);
                Serial.println(": SEND FAILED");
                delay(100);
                continue;
            }

            // Record send timestamp immediately after successful transmission
            uint32_t t_send = millis();
            Serial.print(" - PING to Node ");
            Serial.print(id);
            Serial.print(": sent at ");
            Serial.print(t_send);
            Serial.println(" ms");

            // Attempt to receive response within timeout
            RFMessage response;
            bool received = rf_receive(response, RF_RESPONSE_WAIT_MS);

            // Record receive timestamp after receive attempt
            uint32_t t_recv = millis();
            Serial.print(" - PING to Node ");
            Serial.print(id);
            Serial.print(": received at ");
            Serial.print(t_recv);
            Serial.println(" ms");

            // Validate response: correct sender, receiver, and payload prefix
            if (received &&
                response.from_id == id &&
                response.to_id == NODE_ID &&
                strncmp(response.payload, RF_PONG_PAYLOAD, 4) == 0)
            {
                rtt = t_recv - t_send;
                success = true;
                break;
            }

            delay(100); // Cooldown between attempts
        }

        if (success)
        {
            node_online[id] = true;
            node_rf_latency[id] = rtt / 2;

            Serial.print(" - Node ");
            Serial.print(id);
            Serial.print(": ONLINE | RTT = ");
            Serial.print(rtt);
            Serial.print(" ms | latency ≈ ");
            Serial.print(node_rf_latency[id]);
            Serial.println(" ms");
        }
        else
        {
            node_online[id] = false;

            Serial.print(" - Node ");
            Serial.print(id);
            Serial.println(": OFFLINE");
        }

        delay(200); // Prevent RF congestion
    }

    // Determine if any node was successfully contacted
    bool any_online = false;
    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        if (node_online[id])
        {
            any_online = true;
            break;
        }
    }

    // Compute the average latency across online nodes, and assign it to the offline nodes
    int32_t total_latency = 0;
    int32_t online_count = 0;
    float average_latency = 0.0f;
    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        if (node_online[id])
        {
            total_latency += node_rf_latency[id];
            online_count++;
        }
    }

    if (online_count > 0)
    {
        average_latency = static_cast<float>(total_latency) / online_count;
    }
    else
    {
        average_latency = 0.0f; // No nodes online, set to 0
    }

    // Assign average latency to offline nodes
    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        if (!node_online[id])
        {
            node_rf_latency[id] = static_cast<int32_t>(average_latency);
        }
    }

    // Update status flag
    return any_online;

#else // LEAF NODE
    Serial.println("[COMMUNICATION] <SYNC> Leaf waiting for PING...");

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 500);

        if (!received)
            continue;

        // Ensure message is intended for this node and is a valid PING
        if (msg.to_id != NODE_ID || strncmp(msg.payload, RF_PING_PAYLOAD, 4) != 0)
            continue;

        Serial.print("[COMMUNICATION] <SYNC> Received PING from Node ");
        Serial.println(msg.from_id);

        // Construct and send PONG response
        RFMessage response;
        response.from_id = NODE_ID;
        response.to_id = msg.from_id;
        strncpy(response.payload, RF_PONG_PAYLOAD, sizeof(response.payload) - 1);
        response.payload[sizeof(response.payload) - 1] = '\0';

        rf_stop_listening();
        rf_send(msg.from_id, response, false); // No ACK
        rf_start_listening();

        Serial.println("[COMMUNICATION] <SYNC> PONG sent. Leaf sync complete.");
        break;
    }

    return true;
#endif
}

bool rf_time_sync()
{
#ifdef GATEWAY
    Serial.println("[COMMUNICATION] <SYNC> Starting time sync broadcast to all nodes...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        // Estimate master time just before sending to each node
        uint32_t master_time = static_cast<uint32_t>(Time.estimate_time_ms());

        RFMessage msg;
        msg.from_id = NODE_ID;
        msg.to_id = id;
        snprintf(msg.payload, sizeof(msg.payload), "%s %lu %d", RF_TIME_SYNC_HEADER, master_time, node_rf_latency[id]);

        Serial.print("[COMMUNICATION] <SYNC> Sending time to Node ");
        Serial.print(id);
        Serial.print(": ");
        Serial.println(msg.payload);

        rf_stop_listening();
        bool sent = rf_send(id, msg, false);
        rf_start_listening();

        if (!sent)
        {
            Serial.print("[COMMUNICATION] <SYNC> Failed to send to Node ");
            Serial.println(id);
            continue;
        }

        RFMessage ack;
        bool received = rf_receive(ack, RF_RESPONSE_WAIT_MS);

        if (received &&
            ack.from_id == id &&
            ack.to_id == NODE_ID &&
            strncmp(ack.payload, RF_TIME_ACK_HEADER, strlen(RF_TIME_ACK_HEADER)) == 0)
        {
            uint32_t synced_time = 0;
            sscanf(ack.payload + strlen(RF_TIME_ACK_HEADER) + 1, "%lu", &synced_time);
            Serial.print("[COMMUNICATION] <SYNC> Node ");
            Serial.print(id);
            Serial.print(" ACK received. Synced time = ");
            Serial.println(synced_time);
        }
        else
        {
            Serial.print("[COMMUNICATION] <SYNC> No ACK from Node ");
            Serial.println(id);
        }

        delay(100); // avoid RF congestion
    }

    node_status.node_flags.time_rf_synced = true;
    return true;

#else // LEAF NODE
    Serial.println("[COMMUNICATION] <SYNC> Leaf waiting for TIME_SYNC...");

    uint32_t master_time = 0;
    int latency = 0;
    uint32_t adjusted_time = 0;

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 1000);

        if (!received)
            continue;

        if (msg.to_id != NODE_ID || strncmp(msg.payload, RF_TIME_SYNC_HEADER, strlen(RF_TIME_SYNC_HEADER)) != 0)
            continue;

        // Parse time and latency from payload
        sscanf(msg.payload + strlen(RF_TIME_SYNC_HEADER) + 1, "%lu %d", &master_time, &latency);
        adjusted_time = master_time + latency;

        // Update local time
        Time.set_time_ms(adjusted_time);

        Serial.print("[COMMUNICATION] <SYNC> Time adjusted to ");
        Serial.println(adjusted_time);

        // Send ACK with adjusted time
        RFMessage ack;
        ack.from_id = NODE_ID;
        ack.to_id = msg.from_id;
        snprintf(ack.payload, sizeof(ack.payload), "%s %lu", RF_TIME_ACK_HEADER, adjusted_time);

        rf_stop_listening();
        rf_send(msg.from_id, ack, false);
        rf_start_listening();

        Serial.println("[COMMUNICATION] <SYNC> Time sync ACK sent.");
        break;
    }

    node_status.node_flags.time_rf_synced = (adjusted_time > 0);
    return node_status.node_flags.time_rf_synced;
#endif
}

```

In this project, time synchronization is divided into two categories:  
1. Synchronization with the internet (via NTP)  
2. Synchronization between local devices (via RF communication and RTT)

## 1. Internet Time Synchronization – Network Time Protocol (NTP)

NTP (Network Time Protocol) is used to synchronize local device time with an accurate time source on the internet. The basic principle is:

- The local device sends a request and records the send time `t1`
- The NTP server receives the request and records the server time `t2`
- The server sends back the current time `t3`
- The local device receives the response and records receive time `t4`

Using these timestamps, the local device can estimate the network delay and adjust its own time based on the server's response.

The related function in this project is:

### `sync_time_ntp()`

This function uses the `NTPClient` library to retrieve the current Unix timestamp from the internet. Its core logic includes:

- Initialize and connect to the NTP server
- Retrieve the current UTC time (Unix timestamp)
- Update the global `Time` variable with the retrieved value
- Record the current runtime (e.g., from `millis()`) to enable millisecond-level tracking

This function is called at device startup or when re-synchronization is needed. While it is convenient, it depends on internet access and the precision is limited to seconds on platforms like Arduino.

---

## 2. Local Device Synchronization – Based on RTT and RF Communication

Because NTP precision is limited and requires network connectivity, we implement a high-precision local synchronization mechanism using RF (radio frequency) communication. The idea is:

- One node (gateway) acts as the time reference
- Other nodes (leaf nodes) synchronize their clocks through communication with the gateway
- The process uses RTT (Round Trip Time) to estimate delay

The synchronization process includes two steps:

---

### 1. Checking Node Online Status and RTT Calculation

The gateway sends a `PING` message to each leaf node. Each leaf node responds immediately with a `PONG`. The gateway records:

- `t_send`: time when PING was sent
- `t_recv`: time when PONG was received

The round-trip time (RTT) is:

\[
RTT = t_{recv} - t_{send}
\]

Then the RF latency (one-way delay) is estimated as:

\[
Latency = RTT / 2
\]

The gateway maintains two arrays:

- `node_rf_latency[]`: stores estimated RF delay for each node
- `node_online[]`: records whether each node is online (responding)

For nodes that do not respond, the average latency of the responding nodes is assigned as a fallback.

---

### 2. Synchronizing Time Based on RTT and Reference Time

After calculating delays, the gateway sends a `TIME_SYNC` message to each leaf node. The message includes:

- The current time of the gateway
- The calculated RF latency for that node

Upon receiving the message, each leaf node:

1. Extracts the reference time and delay from the message
2. Computes the adjusted local time as (reference time + delay)
3. Overwrites its local time with the adjusted value
4. Sends an `ACK` message back, including the new time for confirmation

This completes synchronization, and the node's time is now aligned with the gateway's time with millisecond-level accuracy.

---

## Summary

| Synchronization Type | Description | Pros | Cons |
|----------------------|-------------|------|------|
| NTP Sync             | Uses internet to get UTC time | Universal, no local reference required | Limited precision (seconds), network dependent |
| RF Local Sync        | Uses RF communication + RTT calculation | High precision (milliseconds), network independent | Requires communication logic and node coordination |

To balance precision and practicality, this project first uses NTP for initial time setup, followed by RF-based synchronization for high-precision alignment among nodes.
