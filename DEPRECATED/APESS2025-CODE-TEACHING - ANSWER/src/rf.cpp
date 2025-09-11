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

// void rf_check_node_status()
// {
// #ifdef GATEWAY

//     delay(2000); // Allow time for radio to stabilize
//     Serial.println("[RF] Checking node status as GATEWAY...");

//     const char *ping_payload = "PING";
//     const unsigned long timeout_ms = 200;

//     for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
//     {
//         if (node_id == NODE_ID)
//             continue;

//         RFMessage ping_msg;
//         ping_msg.from_id = NODE_ID;
//         ping_msg.to_id = node_id;
//         strncpy(ping_msg.payload, ping_payload, sizeof(ping_msg.payload));
//         ping_msg.timestamp_ms = millis();

//         bool online = rf_send_then_receive(ping_msg, node_id, timeout_ms, 1);
//         node_online[node_id] = online;
//     }

//     // === Summary Output ===
//     Serial.println("[RF] Online Node Summary:");
//     for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
//     {
//         if (node_id == NODE_ID)
//             continue;
//         if (node_online[node_id])
//         {
//             Serial.print("  - Node ");
//             Serial.print(node_id);
//             Serial.println(" is ONLINE.");
//         }
//     }
// #endif

// #ifdef LEAFNODE
//     Serial.println("[RF] Waiting for PING from GATEWAY...");

//     while (true)
//     {
//         RFMessage msg;
//         if (rf_receive(msg, 100))
//         {
//             if (strncmp(msg.payload, "PING", 4) == 0 && msg.to_id == NODE_ID)
//             {
//                 RFMessage reply;
//                 reply.from_id = NODE_ID;
//                 reply.to_id = msg.from_id;
//                 strncpy(reply.payload, "PONG", sizeof(reply.payload));
//                 reply.timestamp_ms = millis();

//                 rf_stop_listening();
//                 rf_send(msg.from_id, reply, false);
//                 rf_start_listening();

//                 Serial.println("[RF] Responded to GATEWAY PING. Ready.");
//                 break; // Exit the loop — proceed with setup
//             }
//         }
//     }
// #endif
// }

// void rf_sync_log_number()
// {
// #ifdef GATEWAY
//     Serial.println("[RF] Syncing LOG_NUMBER to all nodes with ACK...");

//     const int MAX_RETRY = 3;
//     const unsigned long timeout_ms = 200;

//     for (uint8_t node_id = 1; node_id <= NUM_NODES; ++node_id)
//     {
//         if (node_id == NODE_ID)
//             continue;

//         bool ack_received = false;

//         for (int attempt = 1; attempt <= MAX_RETRY && !ack_received; ++attempt)
//         {
//             RFMessage msg;
//             msg.from_id = NODE_ID;
//             msg.to_id = node_id;
//             snprintf(msg.payload, sizeof(msg.payload), "LOG %d", log_number);
//             msg.timestamp_ms = millis();

//             rf_stop_listening();
//             rf_send(node_id, msg, false);
//             rf_start_listening();

//             Serial.print("[GATEWAY] Sent LOG_NUMBER to Node ");
//             Serial.print(node_id);
//             Serial.print(" (Attempt ");
//             Serial.print(attempt);
//             Serial.println(")");

//             // Wait for ACK
//             unsigned long start = millis();
//             while (millis() - start < timeout_ms)
//             {
//                 RFMessage ack_msg;
//                 if (rf_receive(ack_msg, timeout_ms))
//                 {
//                     if (ack_msg.to_id == NODE_ID &&
//                         ack_msg.from_id == node_id &&
//                         strcmp(ack_msg.payload, "ACK_LOG") == 0)
//                     {
//                         Serial.print("[GATEWAY] ACK_LOG received from Node ");
//                         Serial.println(node_id);
//                         ack_received = true;
//                         break;
//                     }
//                 }
//             }
//         }

//         if (!ack_received)
//         {
//             Serial.print("[GATEWAY][WARN] No ACK_LOG from Node ");
//             Serial.println(node_id);
//         }
//     }

// #endif

// #ifdef LEAFNODE
//     Serial.println("[LEAFNODE] Listening for LOG_NUMBER...");

//     unsigned long start_wait = millis();
//     const unsigned long max_wait = 5000;  // Max wait time

//     while (millis() - start_wait < max_wait)
//     {
//         RFMessage msg;
//         if (rf_receive(msg, 100))
//         {
//             if (strncmp(msg.payload, "LOG", 3) == 0 && msg.to_id == NODE_ID)
//             {
//                 int received_log = 0;
//                 sscanf(msg.payload, "LOG %d", &received_log);

//                 log_number = received_log;
//                 save_log_number();

//                 Serial.print("[LEAFNODE] Received LOG_NUMBER = ");
//                 Serial.println(log_number);

//                 // Send ACK
//                 RFMessage ack;
//                 ack.from_id = NODE_ID;
//                 ack.to_id = msg.from_id;
//                 strncpy(ack.payload, "ACK_LOG", sizeof(ack.payload));
//                 ack.timestamp_ms = millis();

//                 rf_stop_listening();
//                 rf_send(msg.from_id, ack, false);
//                 rf_start_listening();

//                 Serial.println("[LEAFNODE] ACK_LOG sent.");
//                 break;
//             }
//         }
//     }
// #endif
// }


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
