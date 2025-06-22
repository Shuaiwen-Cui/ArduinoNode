#include <Arduino.h>
#include "config.hpp"
#include "rf.hpp"
#include "rgbled.hpp"
#include "rf_test.hpp"

#define MAX_RETRY_PER_NODE 10     // Retry limit per node
#define GATEWAY_TIMEOUT_MS 60000 // Timeout for all node responses

void rf_test_gateway()
{
    Serial.println("[TEST] Gateway RF test started.");
    const uint8_t total_nodes = NUM_NODES;
    bool responded[total_nodes + 1] = {false}; // index 0 unused
    uint8_t retries[total_nodes + 1] = {0};
    unsigned long start_time = millis();

    while (millis() - start_time < GATEWAY_TIMEOUT_MS)
    {
        for (uint8_t id = 1; id <= total_nodes; ++id)
        {
            if (responded[id] || retries[id] >= MAX_RETRY_PER_NODE)
                continue;

            RFMessage msg;
            msg.from_id = NODE_ID;
            msg.to_id = id;
            strncpy(msg.payload, "PING", sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0';

            Serial.print("[TEST] Sending PING to Node ");
            Serial.println(id);

            rf_stop_listening();
            bool sent = rf_send(id, msg);
            rf_start_listening();

            if (!sent)
            {
                Serial.println("[TEST] Send failed. Retrying...");
                retries[id]++;
                delay(50);
                continue;
            }

            RFMessage response;
            bool received = rf_receive(response, 100);

            if (received && response.from_id == id &&
                strncmp(response.payload, "PONG", 4) == 0)
            {
                responded[id] = true;
                Serial.print("[TEST] Received PONG from Node ");
                Serial.println(id);
            }
            else
            {
                retries[id]++;
                Serial.print("[TEST] No response from Node ");
                Serial.println(id);
            }

            delay(50); // Prevent flooding
        }

        bool all_done = true;
        for (uint8_t id = 1; id <= total_nodes; ++id)
        {
            if (!responded[id])
            {
                all_done = false;
                break;
            }
        }

        if (all_done)
            break;
    }

    Serial.println("[TEST] Gateway test complete.");
    for (uint8_t id = 1; id <= total_nodes; ++id)
    {
        Serial.print("Node ");
        Serial.print(id);
        Serial.print(": ");
        Serial.println(responded[id] ? "PONG received" : "No response");
    }
}

void rf_test_leaf()
{
    Serial.println("[TEST] Leaf RF test started. Waiting for PING...");

    while (true)
    {
        RFMessage msg;
        bool received = rf_receive(msg, 500);

        if (!received)
            continue;

        if (msg.to_id != NODE_ID || strncmp(msg.payload, "PING", 4) != 0)
            continue;

        Serial.print("[TEST] Received PING from Node ");
        Serial.println(msg.from_id);

        RFMessage response;
        response.from_id = NODE_ID;
        response.to_id = msg.from_id;
        strncpy(response.payload, "PONG", sizeof(response.payload) - 1);
        response.payload[sizeof(response.payload) - 1] = '\0';

        rf_stop_listening();
        rf_send(msg.from_id, response);
        rf_start_listening();

        rgbled_set_all(CRGB::Green);
        Serial.println("[TEST] Sent PONG. Leaf test complete.");
        break;
    }
}
