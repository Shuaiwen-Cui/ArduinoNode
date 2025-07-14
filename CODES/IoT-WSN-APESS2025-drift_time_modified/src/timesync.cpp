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

bool check_rf_online_and_latency()
{
#ifdef GATEWAY
    Serial.println("[CHECK] Master checking node connectivity...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        const uint8_t MAX_ATTEMPTS = 5;
        uint32_t rtt_samples[MAX_ATTEMPTS];
        uint8_t sample_count = 0;

        for (uint8_t attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
        {
            // --- Step 1: Send PING ---
            RFMessage msg;
            msg.from_id = NODE_ID;
            msg.to_id = id;
            strncpy(msg.payload, RF_PING_PAYLOAD, sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0';

            rf_stop_listening();
            bool sent = rf_send(id, msg, false);
            uint32_t t_send = millis(); // Record time of send
            rf_start_listening();

            if (!sent)
                continue;

            // --- Step 2: Wait for PONG ---
            RFMessage response;
            bool received = rf_receive(response, RF_RESPONSE_WAIT_MS);
            uint32_t t_recv = millis(); // Time we received

            if (received && response.from_id == id &&
                response.to_id == NODE_ID &&
                strncmp(response.payload, RF_PONG_PAYLOAD, 4) == 0)
            {
                uint32_t rtt = t_recv - t_send;
                rtt_samples[sample_count++] = rtt;

                Serial.print("   ↳ Sample ");
                Serial.print(sample_count);
                Serial.print(": RTT = ");
                Serial.print(rtt);
                Serial.println(" ms");
            }

            delay(150); // Prevent congestion
        }

        // --- Step 3: Analyze samples ---
        if (sample_count > 0)
        {
            // Sort rtt_samples[0...sample_count-1]
            for (uint8_t i = 0; i < sample_count - 1; ++i)
            {
                for (uint8_t j = i + 1; j < sample_count; ++j)
                {
                    if (rtt_samples[j] < rtt_samples[i])
                    {
                        uint32_t temp = rtt_samples[i];
                        rtt_samples[i] = rtt_samples[j];
                        rtt_samples[j] = temp;
                    }
                }
            }

            uint32_t median_rtt = rtt_samples[sample_count / 2];
            node_rf_latency[id] = median_rtt / 2;
            node_online[id] = true;

            Serial.print(" - Node ");
            Serial.print(id);
            Serial.print(": ONLINE | Median RTT = ");
            Serial.print(median_rtt);
            Serial.print(" ms | Estimated latency = ");
            Serial.print(node_rf_latency[id]);
            Serial.println(" ms");
        }
        else
        {
            node_online[id] = false;
            Serial.print(" - Node ");
            Serial.print(id);
            Serial.println(": OFFLINE or no valid response.");
        }

        delay(200); // Between nodes
    }

    return std::any_of(node_online + 1, node_online + NUM_NODES + 1, [](bool online)
                       { return online; });

#elif defined(LEAFNODE)
    Serial.println("[CHECK] Leaf waiting for PING...");

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 1000); 

        if (!received)
            continue;

        if (msg.to_id != NODE_ID || strncmp(msg.payload, RF_PING_PAYLOAD, 4) != 0)
            continue;

        RFMessage response;
        response.from_id = NODE_ID;
        response.to_id = msg.from_id;
        strncpy(response.payload, RF_PONG_PAYLOAD, sizeof(response.payload) - 1);
        response.payload[sizeof(response.payload) - 1] = '\0';

        rf_stop_listening();
        rf_send(msg.from_id, response, false);
        rf_start_listening();

        Serial.println("[CHECK] PONG sent.");
        break;
    }

    return true;
#endif
}

bool rf_sync_drift()
{
#ifdef GATEWAY
    Serial.println("[SYNC] GATEWAY: Starting drift sync...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {
        Serial.print("[SYNC] Target Node ");
        Serial.print(id);
        Serial.println("...");

        for (uint8_t i = 0; i < DRIFT_SYNC_SAMPLES; ++i)
        {
            RFMessage msg;
            msg.from_id = NODE_ID;
            msg.to_id = id;

            // === Use millis() instead of Time.estimate_time_ms() ===
            uint64_t now = millis(); // updated in each round
            uint32_t hi = now >> 32;
            uint32_t lo = now & 0xFFFFFFFF;

            snprintf(msg.payload, sizeof(msg.payload), "%s %lu %lu", DRIFT_SYNC_TAG, hi, lo);

            rf_stop_listening();
            bool sent = rf_send(id, msg, false);
            rf_start_listening();

            if (sent)
            {
                Serial.print("   ↳ Sent TSYNC ");
                Serial.print(i + 1);
                Serial.print(" to Node ");
                Serial.print(id);
                Serial.print(" : ");
                Serial.println(msg.payload);
            }
            else
            {
                Serial.print("   ↳ Failed to send TSYNC to Node ");
                Serial.println(id);
            }

            // Optional ACK (non-blocking if not received)
            RFMessage ack;
            bool ack_received = rf_receive(ack, RF_ACK_TIMEOUT_MS);
            if (ack_received &&
                ack.from_id == id &&
                ack.to_id == NODE_ID &&
                strncmp(ack.payload, "ACK_DRIFT", 9) == 0)
            {
                Serial.println("   ↳ ACK received.");
            }

            delay(DRIFT_SYNC_INTERVAL_MS);
        }
    }

#elif defined(LEAFNODE)
    Serial.println("[SYNC] LEAF: Listening for TSYNC drift messages...");

    uint64_t tg[DRIFT_SYNC_SAMPLES] = {0}; // Master timestamps
    uint32_t tl[DRIFT_SYNC_SAMPLES] = {0}; // Local timestamps
    uint8_t count = 0;

    while (count < DRIFT_SYNC_SAMPLES)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 10000);
        if (!received)
            continue;

        if (msg.to_id != NODE_ID ||
            strncmp(msg.payload, DRIFT_SYNC_TAG, strlen(DRIFT_SYNC_TAG)) != 0)
            continue;

        // Parse 64-bit master timestamp
        uint32_t hi = 0, lo = 0;
        int parsed = sscanf(msg.payload + strlen(DRIFT_SYNC_TAG) + 1, "%lu %lu", &hi, &lo);
        if (parsed != 2)
        {
            Serial.println("[SYNC] Failed to parse TSYNC payload.");
            continue;
        }

        tg[count] = ((uint64_t)hi << 32) | lo;
        tl[count] = millis();

        Serial.print("   ↳ Received TSYNC ");
        Serial.print(count + 1);
        Serial.print(" | tg = ");
        Serial.print(tg[count]);
        Serial.print(" | tl = ");
        Serial.println(tl[count]);

        // Send optional ACK
        RFMessage ack;
        ack.from_id = NODE_ID;
        ack.to_id = msg.from_id;
        snprintf(ack.payload, sizeof(ack.payload), "ACK_DRIFT");

        rf_stop_listening();
        rf_send(msg.from_id, ack, false);
        rf_start_listening();

        count++;
    }

    // === Estimate drift ratio via median slope ===
    float slopes[DRIFT_SYNC_SAMPLES - 1];
    for (uint8_t i = 1; i < DRIFT_SYNC_SAMPLES; ++i)
    {
        uint64_t delta_tg = tg[i] - tg[0];
        int32_t delta_tl = (int32_t)(tl[i] - tl[0]);

        if (delta_tl <= 0)
            slopes[i - 1] = 1.0f; // fallback
        else
            slopes[i - 1] = (float)delta_tg / (float)delta_tl;
    }

    std::sort(slopes, slopes + (DRIFT_SYNC_SAMPLES - 1));
    drift_ratio = slopes[(DRIFT_SYNC_SAMPLES - 1) / 2];

    Serial.print("[SYNC] Updated drift ratio = ");
    Serial.println(drift_ratio, 8);

    return true;

#else
#error "Define GATEWAY or LEAFNODE"
#endif
}

bool rf_time_sync()
{
#ifdef GATEWAY
    Serial.println("[COMMUNICATION] <SYNC> Starting time sync broadcast to all nodes...");

    for (uint8_t id = 1; id <= NUM_NODES; ++id)
    {

        RFMessage msg;
        msg.from_id = NODE_ID;
        msg.to_id = id;
        uint64_t adjusted_time = Time.estimate_time_ms() + node_rf_latency[id];
        uint32_t high = adjusted_time >> 32;
        uint32_t low = adjusted_time & 0xFFFFFFFF;
        snprintf(msg.payload, sizeof(msg.payload), "%s %lu %lu", RF_TIME_SYNC_HEADER, high, low);

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
            Serial.print("[COMMUNICATION] <SYNC> Node ");
            Serial.print(id);
            Serial.println(" ACK received.");
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

    uint64_t adjusted_time = 0;

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 1000);

        if (!received)
            continue;

        if (msg.to_id != NODE_ID || strncmp(msg.payload, RF_TIME_SYNC_HEADER, strlen(RF_TIME_SYNC_HEADER)) != 0)
            continue;

        Serial.print("[SYNC] Received payload: ");
        Serial.println(msg.payload);

        uint32_t high = 0, low = 0;
        int parsed = sscanf(msg.payload + strlen(RF_TIME_SYNC_HEADER) + 1, "%lu %lu", &high, &low);
        if (parsed != 2)
        {
            Serial.println("[SYNC] Failed to parse adjusted_time from payload.");
            continue;
        }
        uint64_t adjusted_time = ((uint64_t)high << 32) | low;

        Time.set_time_ms(adjusted_time);
        Time.last_update_ms = adjusted_time;
        Time.last_update_epoch = adjusted_time / 1000;
        Time.mcu_base_ms = millis();
        node_status.node_flags.time_rf_synced = true;

        Serial.print("[SYNC] Local time updated to ");
        Time.print();

        RFMessage ack;
        ack.from_id = NODE_ID;
        ack.to_id = msg.from_id;
        snprintf(ack.payload, sizeof(ack.payload), "%s", RF_TIME_ACK_HEADER);

        rf_stop_listening();
        rf_send(msg.from_id, ack, false);
        rf_start_listening();

        Serial.println("[COMMUNICATION] <SYNC> Time sync ACK sent.");
        break;
    }

    return true;
#endif
}
