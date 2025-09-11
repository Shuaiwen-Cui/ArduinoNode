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
