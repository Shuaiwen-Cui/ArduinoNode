#include "rf.hpp"
#include <SPI.h>

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
    Serial.println("[RF] Checking node status as GATEWAY...");

    const char *ping_payload = "PING";
    const unsigned long timeout_ms = 200;

    for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
    {
        if (node_id == NODE_ID)
            continue;

        RFMessage ping_msg;
        ping_msg.from_id = NODE_ID;
        ping_msg.to_id = node_id;
        strncpy(ping_msg.payload, ping_payload, sizeof(ping_msg.payload));
        ping_msg.timestamp_ms = millis();

        bool online = rf_send_then_receive(ping_msg, node_id, timeout_ms, 1);
        node_online[node_id] = online;
    }

    // === Summary Output ===
    Serial.println("[RF] Online Node Summary:");
    for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
    {
        if (node_id == NODE_ID)
            continue;
        if (node_online[node_id])
        {
            Serial.print("  - Node ");
            Serial.print(node_id);
            Serial.println(" is ONLINE.");
        }
    }
#endif

#ifdef LEAFNODE
    Serial.println("[RF] Waiting for PING from GATEWAY...");

    while (true)
    {
        RFMessage msg;
        if (rf_receive(msg, 100))
        {
            if (strncmp(msg.payload, "PING", 4) == 0 && msg.to_id == NODE_ID)
            {
                RFMessage reply;
                reply.from_id = NODE_ID;
                reply.to_id = msg.from_id;
                strncpy(reply.payload, "PONG", sizeof(reply.payload));
                reply.timestamp_ms = millis();

                rf_stop_listening();
                rf_send(msg.from_id, reply, false);
                rf_start_listening();

                Serial.println("[RF] Responded to GATEWAY PING. Ready.");
                break; // Exit the loop — proceed with setup
            }
        }
    }
#endif
}
