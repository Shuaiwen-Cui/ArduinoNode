#include <Arduino.h>
#include "config.hpp"
#include "rf.hpp"
#include "rgbled.hpp"

uint64_t now_unix_ms = 0;

#ifdef GATEWAY
const uint8_t gateway_id = NODE_ID;
const size_t leafnode_count = NUM_NODES - 1;
unsigned long last_send_time = 0;
unsigned long counter = 0;

// 获取子节点 ID（排除 gateway 自己）
uint8_t get_leafnode_id(size_t index) {
    return (index < gateway_id - 1) ? index + 1 : index + 2;
}
#endif

void setup() {
    Serial.begin(115200);
    while (!Serial);

    rgbled_init();
    rgbled_set_all(CRGB::Orange);
    delay(500);

    if (!rf_init()) {
        rgbled_set_all(CRGB::Red);
        while (1);  // 停止运行
    }

    rgbled_set_all(CRGB::Green);
    delay(500);
    rgbled_set_all(CRGB::White);
}

void loop() {
#ifdef GATEWAY
    unsigned long now = millis();
    if (now - last_send_time >= 5000) {
        last_send_time = now;
        counter++;

        uint8_t target_id = get_leafnode_id(counter % leafnode_count);
        char payload[22];
        snprintf(payload, sizeof(payload), "CNT_%lu", counter);

        RFMessage msg;
        msg.from_id = NODE_ID;
        msg.to_id = target_id;
        strncpy(msg.payload, payload, sizeof(msg.payload));
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        msg.timestamp_ms = millis();

        bool sent = rf_send(target_id, msg);
        Serial.print("[GATEWAY] Sent ");
        Serial.print(payload);
        Serial.print(" to node ");
        Serial.println(target_id);

        if (!sent) {
            Serial.println("[GATEWAY] Send failed.");
        }
    }
#endif

#ifdef LEAFNODE
    RFMessage msg;
    if (rf_receive(msg, 50)) {
        if (msg.to_id != NODE_ID && msg.to_id != 0xFF) return;

        Serial.print("[LEAFNODE ");
        Serial.print(NODE_ID);
        Serial.print("] Received: ");
        Serial.println(msg.payload);

        // Parse number from CNT_<number>
        if (strncmp(msg.payload, "CNT_", 4) == 0) {
            int num = atoi(msg.payload + 4);
            switch (num % 7) {
                case 0: rgbled_set_all(CRGB::Red); break;
                case 1: rgbled_set_all(CRGB::Green); break;
                case 2: rgbled_set_all(CRGB::Blue); break;
                case 3: rgbled_set_all(CRGB::Yellow); break;
                case 4: rgbled_set_all(CRGB::Cyan); break;
                case 5: rgbled_set_all(CRGB::Purple); break;
                case 6: rgbled_set_all(CRGB::White); break;
            }
        }
    }
#endif
}
