#include "timesync.hpp"
#include "config.hpp"
#include "time.hpp"
#include "rf.hpp"
#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 28800, 60000);

bool node_online[NUM_NODES + 1] = {false};
int32_t node_rf_latency[NUM_NODES + 1] = {0};
float node_drift_ratio[NUM_NODES + 1] = {0};

bool sync_time_ntp()
{
    timeClient.begin();
    const uint64_t MIN_VALID_EPOCH = 1735689600; // 2025-01-01 00:00:00 UTC
    bool success = false;

    for (int attempt = 1; attempt <= 5; ++attempt)
    {
        if (!timeClient.update())
        {
            Serial.print("[COMMUNICATION] <NTP> Attempt ");
            Serial.print(attempt);
            Serial.println(": Failed to get NTP time.");
            delay(1000);
            continue;
        }

        uint64_t epoch = timeClient.getEpochTime();
        if (epoch < MIN_VALID_EPOCH)
        {
            Serial.print("[COMMUNICATION] <NTP> Attempt ");
            Serial.print(attempt);
            Serial.print(": Invalid epoch = ");
            Serial.println(epoch);
            delay(1000);
            continue;
        }

        // === Valid time received, set local time ===
        uint16_t current_millis = millis();

        Time.set_time_epoch(epoch);
        Time.last_update_epoch = epoch;
        Time.last_update_ms = current_millis % 1000 + (epoch * 1000); // Convert to ms, keeping current millis
        Time.mcu_base_ms = current_millis;
        Time.mcu_time_ms = current_millis;
        Time.delta_ms = 0;
        Serial.print("[COMMUNICATION] <NTP> Calendar time set to: ");
        Time.print(); // Print the time to Serial for debugging

        Serial.print("[COMMUNICATION] <NTP> Synchronized UNIX epoch: ");
        Serial.println(epoch);
        Serial.print("[COMMUNICATION] <NTP> Current time: ");
        Time.print();

        success = true;
        break;
    }

    if (!success)
    {
        Serial.println("[COMMUNICATION] <NTP> Final NTP sync failed after 5 attempts.");
    }

    return success;
}

bool sync_check_rf_online()
{
#ifdef GATEWAY
    Serial.println("[COMMUNICATION] <SYNC> Master checking nodes...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        float sum_drift = 0.0f;
        int32_t sum_offset = 0;
        uint8_t count = 0;

        for (uint8_t i = 0; i < PONG_REPETITIONS; ++i)
        {
            // === Step 1: Send PING ===
            RFMessage msg;
            msg.from_id = NODE_ID;
            msg.to_id = id;
            strncpy(msg.payload, RF_PING_PAYLOAD, sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0';

            rf_stop_listening();
            bool sent = rf_send(id, msg, false);
            uint32_t t1 = millis();
            rf_start_listening();

            if (!sent)
            {
                Serial.print(" - Node ");
                Serial.print(id);
                Serial.println(": SEND FAILED (single attempt)");
                continue;
            }

            Serial.print(" - Sent PING to Node ");
            Serial.print(id);
            Serial.print(" [Attempt ");
            Serial.print(i + 1);
            Serial.print("] at ");
            Serial.println(t1);

            // === Step 2: Wait for PONG ===
            RFMessage response;
            bool received = rf_receive(response, RF_RESPONSE_WAIT_MS);

            uint32_t t4 = millis(); // time when PONG is received

            if (!received)
                continue;

            if (response.from_id == id &&
                response.to_id == NODE_ID &&
                strncmp(response.payload, RF_PONG_PAYLOAD, 4) == 0)
            {
                uint32_t t2 = 0, t3 = 0;
                int parsed = sscanf(response.payload + strlen(RF_PONG_PAYLOAD) + 1, "%lu %lu", &t2, &t3);

                if (parsed == 2 && t3 > t2)
                {
                    uint32_t delta_master = t4 - t1;
                    uint32_t delta_node   = t3 - t2;

                    if (delta_node == 0)
                        continue;

                    float drift  = (float)delta_master / (float)delta_node;
                    int32_t offset = (int32_t)((delta_master - delta_node) / 2);

                    sum_drift  += drift;
                    sum_offset += offset;
                    count++;

                    Serial.print("   ↳ Sample ");
                    Serial.print(count);
                    Serial.print(": Drift = ");
                    Serial.print(drift, 8);
                    Serial.print(" | Offset = ");
                    Serial.print(offset);
                    Serial.println(" ms");

                    Serial.print("     t1 = "); Serial.println(t1);
                    Serial.print("     t2 = "); Serial.println(t2);
                    Serial.print("     t3 = "); Serial.println(t3);
                    Serial.print("     t4 = "); Serial.println(t4);
                }
            }

            delay(PONG_INTERVAL_MS); // avoid congestion
        }

        if (count > 0)
        {
            node_drift_ratio[id] = sum_drift / count;
            node_rf_latency[id] = sum_offset / count;
            node_online[id] = true;

            Serial.print(" - Node ");
            Serial.print(id);
            Serial.print(": ONLINE | Avg Drift = ");
            Serial.print(node_drift_ratio[id], 8);
            Serial.print(" | Avg Offset = ");
            Serial.println(node_rf_latency[id]);
        }
        else
        {
            node_online[id] = false;
            Serial.print(" - Node ");
            Serial.print(id);
            Serial.println(": OFFLINE or no valid replies.");
        }

        delay(300);
    }

    return std::any_of(node_online + 1, node_online + NUM_NODES + 1, [](bool online)
                       { return online; });

#else // === LEAF NODE ===
    Serial.println("[COMMUNICATION] <SYNC> Leaf waiting for PING...");

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 5000);
        if (!received) continue;

        if (msg.to_id != NODE_ID || strncmp(msg.payload, RF_PING_PAYLOAD, 4) != 0)
            continue;

        uint32_t t2 = millis();  // time when PING is received

        Serial.print("[SYNC] Received PING from Node ");
        Serial.println(msg.from_id);

        // Slight delay to simulate processing, helps spread t3 values
        delay(20);

        uint32_t t3 = millis();  // time when PONG is sent

        RFMessage response;
        response.from_id = NODE_ID;
        response.to_id = msg.from_id;
        snprintf(response.payload, sizeof(response.payload), "%s %lu %lu", RF_PONG_PAYLOAD, t2, t3);

        rf_stop_listening();
        rf_send(msg.from_id, response, false);
        rf_start_listening();

        Serial.print("[SYNC] PONG sent | t2 = ");
        Serial.print(t2);
        Serial.print(", t3 = ");
        Serial.println(t3);
    }

    return true;
#endif
}


bool rf_time_sync()
{
#ifdef GATEWAY
    Serial.println("[SYNC] GATEWAY: Starting time sync to all nodes...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        // === Step 1: Send drift ratio ===
        RFMessage drift_msg;
        drift_msg.from_id = NODE_ID;
        drift_msg.to_id = id;

        int32_t drift_int = (int32_t)(node_drift_ratio[id] * 1e8f);
        snprintf(drift_msg.payload, sizeof(drift_msg.payload), "DRIFT %ld", drift_int);

        rf_stop_listening();
        bool drift_sent = rf_send(id, drift_msg, false);
        rf_start_listening();

        if (!drift_sent)
        {
            Serial.print("[SYNC] Failed to send DRIFT to Node ");
            Serial.println(id);
            continue;
        }

        Serial.print("[SYNC] Sent DRIFT to Node ");
        Serial.print(id);
        Serial.print(" : ");
        Serial.println(drift_msg.payload);
        delay(50);

        // === Step 2: Send adjusted time ===
        RFMessage time_msg;
        time_msg.from_id = NODE_ID;
        time_msg.to_id = id;

        uint64_t adjusted_time = Time.estimate_time_ms() + node_rf_latency[id];
        uint32_t high = adjusted_time >> 32;
        uint32_t low = adjusted_time & 0xFFFFFFFF;
        snprintf(time_msg.payload, sizeof(time_msg.payload), "%s %lu %lu", RF_TIME_SYNC_HEADER, high, low);

        rf_stop_listening();
        bool time_sent = rf_send(id, time_msg, false);
        rf_start_listening();

        if (!time_sent)
        {
            Serial.print("[SYNC] Failed to send TIME_SYNC to Node ");
            Serial.println(id);
            continue;
        }

        Serial.print("[SYNC] Sent TIME_SYNC to Node ");
        Serial.print(id);
        Serial.print(" : ");
        Serial.println(time_msg.payload);

        // === Step 3: Wait for ACK ===
        RFMessage ack;
        bool received = rf_receive(ack, RF_RESPONSE_WAIT_MS);
        if (received &&
            ack.from_id == id &&
            ack.to_id == NODE_ID &&
            strncmp(ack.payload, RF_TIME_ACK_HEADER, strlen(RF_TIME_ACK_HEADER)) == 0)
        {
            Serial.print("[SYNC] Node ");
            Serial.print(id);
            Serial.println(" ACK received.");
        }
        else
        {
            Serial.print("[SYNC] No ACK from Node ");
            Serial.println(id);
        }

        delay(100);
    }

    node_status.node_flags.time_rf_synced = true;
    return true;

#else
    Serial.println("[SYNC] LEAF: Waiting for DRIFT and TIME_SYNC...");

    float received_drift = 1.0f;
    uint64_t adjusted_time = 0;
    bool drift_received = false;
    bool time_received = false;

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 1000);
        if (!received)
            continue;

        if (msg.to_id != NODE_ID)
            continue;

        // === Receive DRIFT ===
        if (strncmp(msg.payload, "DRIFT", 5) == 0)
        {
            int32_t drift_int = 100000000;
            sscanf(msg.payload + 6, "%ld", &drift_int);
            received_drift = (float)drift_int / 1e8f;
            drift_ratio = received_drift;
            drift_received = true;

            Serial.print("[SYNC] Received DRIFT ratio: ");
            Serial.println(drift_ratio, 8);
        }

        // === Receive TIME_SYNC ===
        else if (strncmp(msg.payload, RF_TIME_SYNC_HEADER, strlen(RF_TIME_SYNC_HEADER)) == 0)
        {
            uint32_t high = 0, low = 0;
            int parsed = sscanf(msg.payload + strlen(RF_TIME_SYNC_HEADER) + 1, "%lu %lu", &high, &low);
            if (parsed != 2)
            {
                Serial.println("[SYNC] Failed to parse TIME_SYNC payload.");
                continue;
            }

            adjusted_time = ((uint64_t)high << 32) | low;
            time_received = true;

            if (drift_received)
            {
                Time.set_time_ms(adjusted_time);
                Time.last_update_ms = adjusted_time;
                Time.last_update_epoch = adjusted_time / 1000;
                Time.mcu_base_ms = millis();
                node_status.node_flags.time_rf_synced = true;

                Serial.print("[SYNC] Drift ratio set to: ");
                Serial.println(drift_ratio, 8);
                Serial.print("[SYNC] Local time synchronized to: ");
                Time.print();

                // === Send ACK ===
                RFMessage ack;
                ack.from_id = NODE_ID;
                ack.to_id = msg.from_id;
                snprintf(ack.payload, sizeof(ack.payload), "%s", RF_TIME_ACK_HEADER);

                rf_stop_listening();
                rf_send(msg.from_id, ack, false);
                rf_start_listening();

                Serial.println("[SYNC] Sent TIME_SYNC ACK.");
                break;
            }
        }
    }

    return true;
#endif
}
