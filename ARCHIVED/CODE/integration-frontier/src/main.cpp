#include <Arduino.h>
#include "config.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "time.hpp"
#include "sensing.hpp"
#include "sdcard.hpp"
#include "rgbled.hpp"
#include "radiofrequency.hpp"

unsigned long last_mqtt_time = 0;
const unsigned long mqtt_interval = 100;

// === Shared packet structure ===
struct SimplePacket
{
    uint16_t node_id;
    uint32_t counter;
};

// === Shared state ===
uint32_t counter = 0;

#ifdef GATEWAY
uint16_t current_node = 1;
const uint16_t total_nodes = 4;
#endif

void setup()
{
  delay(3000);
  Serial.begin(115200);
  while (!Serial)
    ;

  rgbled_init();
  rgbled_set_all(CRGB::Yellow);

  connect_to_wifi();
  time_init();

  while (!sync_time_ntp(global_time))
  {
    Serial.println("[Setup] NTP synchronization failed. Retrying...");
    delay(2000);
  }
  Serial.println("[Setup] Time synchronized via NTP.");

  mqtt_setup();
  mpu_init();

  if (!sdcard_init(10))
  {
    Serial.println("[Main] SD card setup failed.");
    while (1)
      ;
  }

  Serial.println("=== Starting RF Node ===");
#ifdef GATEWAY
  Serial.println("[Role] GATEWAY");
#else
  Serial.print("[Role] LEAFNODE ID = ");
  Serial.println(NODE_ID);
#endif

  rf_init();
  Serial.println("[RF] Radio frequency module initialized.");

  rgbled_set_all(CRGB::Green);
  Serial.println("[Main] Node initialization finished.");
}

void loop()
{

  unsigned long now = millis();
  if (now - last_mqtt_time >= mqtt_interval)
  {
    last_mqtt_time = now;
    mqtt_loop();
  }

  if (cmd_flg_ntp)
  {
    while (!sync_time_ntp(global_time))
    {
      Serial.println("[NTP] Sync failed. Retrying...");
      delay(2000);
    }
    Serial.println("[NTP] Sync successful.");
    cmd_flg_ntp = false;
    mqtt_enabled = true;
  }

  if (cmd_flg_scheduled_sensing)
  {
    Serial.println("[CMD_SENSING] Processing scheduled sensing command...");
    cmd_flg_scheduled_sensing = false;

    if (sensing_prepare(parsed_start_time, parsed_freq, parsed_duration))
    {
      Serial.println("[CMD_SENSING] Sensing task prepared successfully.");
      parsed_start_time.print();
    }
    else
    {
      Serial.println("[CMD_SENSING] Failed to prepare sensing.");
    }
  }

  if (cmd_flg_retrieval)
  {
    cmd_flg_retrieval = false;

    if (!mqtt_client.connected())
    {
      Serial.println("[MQTT] Not connected. Attempting reconnect...");
      mqtt_setup();
      delay(1000);
      if (!mqtt_client.connected())
      {
        Serial.println("[MQTT] Reconnect failed. Aborting retrieval.");
        return;
      }
    }

    File file = SD.open(retrieval_filename, FILE_READ);
    if (!file)
    {
      Serial.print("[Error] File not found: ");
      Serial.println(retrieval_filename);
      return;
    }

    Serial.print("[Retrieval] Reading file: ");
    Serial.println(retrieval_filename);

    size_t total_size = file.size();
    size_t bytes_sent = 0;
    size_t chunk_size = 850;  // updated chunk size
    size_t chunk_index = 1;
    size_t chunk_total = (total_size + chunk_size - 1) / chunk_size;

    char prefix[32];
    snprintf(prefix, sizeof(prefix), "%s", retrieval_filename + 1); // Remove leading '/'
    char topic[64];

    while (file.available())
    {
      char buffer[851]; // chunk_size + 1 for null terminator
      size_t len = file.readBytes(buffer, chunk_size);
      buffer[len] = '\0';

      snprintf(topic, sizeof(topic), "%s[%d/%d]:", prefix, chunk_index, chunk_total);
      String payload = String(topic) + String(buffer);

      Serial.print("[Debug] Payload length: ");
      Serial.println(payload.length());

      bool ok = mqtt_client.publish(MQTT_TOPIC_PUB, payload.c_str());
      if (!ok)
      {
        Serial.print("[Error] Failed to send chunk ");
        Serial.println(chunk_index);
      }

      bytes_sent += len;
      chunk_index++;

      Serial.print("[Retrieval] Sent ");
      Serial.print(bytes_sent);
      Serial.print(" / ");
      Serial.print(total_size);
      Serial.println(" bytes");

      mqtt_loop(); // feed the MQTT client
      delay(100);  // prevent flooding
    }

    file.close();

    // Final marker
    String done_msg = String(prefix) + "[done]";
    mqtt_client.publish(MQTT_TOPIC_PUB, done_msg.c_str());
    Serial.println("[MQTT] File upload completed.");
  }

  sensing_update();

  if (sensing_is_full())
  {
    sensing_set_flag(false);
    sensing_flush();
  }
}

// void loop()
// {
// #ifdef GATEWAY
//     rf_set_peer_address(current_node);
//     delay(10);  // allow pipe switch

//     Serial.print("[GATEWAY] Polling node ");
//     Serial.println(current_node);

//     SimplePacket pkt;
//     if (rf_receive(&pkt, sizeof(pkt)))
//     {
//         Serial.print("[GATEWAY] Received from Node ");
//         Serial.print(pkt.node_id);
//         Serial.print(" | Counter = ");
//         Serial.println(pkt.counter);
//     }
//     else
//     {
//         Serial.println("[GATEWAY] No response.");
//     }

//     current_node++;
//     if (current_node > total_nodes)
//         current_node = 1;

//     delay(1000);

// #else
//     SimplePacket pkt;
//     pkt.node_id = NODE_ID;
//     pkt.counter = counter++;

//     rf_send(&pkt, sizeof(pkt));
//     delay(2000);
// #endif
// }