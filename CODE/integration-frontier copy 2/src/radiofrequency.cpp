#include "radiofrequency.hpp"
#include "config.hpp"
#include <cstring>

#define PIPE_ADDR_LEN 5
RF24 radio(9, 8);  // CE, CSN

uint8_t rf_address[PIPE_ADDR_LEN + 1] = {0};  // Local address
uint8_t peer_address[PIPE_ADDR_LEN + 1] = {0}; // Peer address

// Format address like "N001", "N100" (5 bytes)
void format_address(uint8_t *buf, uint16_t id)
{
    snprintf((char *)buf, PIPE_ADDR_LEN + 1, "N%03d", id);  // ensures null termination
}

void rf_init()
{
    Serial.println("[RF] Initializing nRF24L01...");

    if (!radio.begin())
    {
        Serial.println("[RF] Initialization failed! Check wiring and power.");
        while (true)
            delay(1000);
    }

    radio.setChannel(108);
    radio.setPALevel(RF24_PA_HIGH);
    radio.setDataRate(RF24_250KBPS);
    radio.setCRCLength(RF24_CRC_16);
    radio.setRetries(5, 15);

    format_address(rf_address, NODE_ID);

#ifdef GATEWAY
    Serial.print("[RF] GATEWAY ID: ");
    Serial.println((char *)rf_address);

    radio.openReadingPipe(1, rf_address);
    radio.startListening();

#elif defined(LEAFNODE)
    format_address(peer_address, 100); // Gateway is always node 100
    Serial.print("[RF] LEAFNODE sending to ");
    Serial.println((char *)peer_address);

    radio.openWritingPipe(peer_address);
    radio.openReadingPipe(1, rf_address);
    radio.stopListening();

#endif

    Serial.println("[RF] Initialization complete.");
}

void rf_set_peer_address(uint16_t node_id)
{
#ifdef GATEWAY
    format_address(peer_address, node_id);
    radio.stopListening();
    radio.openWritingPipe(peer_address);
    Serial.print("[RF] GATEWAY targeting peer: ");
    Serial.println((char *)peer_address);
#endif
}

bool rf_send(const void *data, size_t len)
{
#ifdef LEAFNODE
    radio.stopListening();  // ensure TX mode
    radio.flush_tx();       // clear buffer

    Serial.print("[RF] Sending to ");
    Serial.print((char *)peer_address);
    Serial.print(" | Size: ");
    Serial.print(len);
    Serial.print(" | Data[0]: ");
    Serial.println(((uint8_t *)data)[0]);

    bool ok = radio.write(data, len);
    Serial.println(ok ? "[RF] Send OK (ACK received)" : "[RF] Send Failed (No ACK)");
    return ok;
#else
    return false;
#endif
}

bool rf_receive(void *data, size_t len)
{
#ifdef GATEWAY
    radio.startListening();
    unsigned long start = millis();
    while (millis() - start < 200)
    {
        if (radio.available())
        {
            radio.read(data, len);
            Serial.print("[RF] Received packet | Size: ");
            Serial.print(len);
            Serial.print(" | Data[0]: ");
            Serial.println(((uint8_t *)data)[0]);
            return true;
        }
    }
    Serial.println("[RF] No data received within timeout.");
#endif
    return false;
}
