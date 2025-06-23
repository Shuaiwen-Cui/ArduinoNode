#pragma once
#include <Arduino.h>
#include <RF24.h>

/* RF Channel and Pipe Configuration */
#define RF_CHANNEL 108
#define RF_PIPE_BASE 0xF0F0F0F000LL

/* Message Structure */
struct RFMessage
{
    uint8_t from_id;
    uint8_t to_id;
    char payload[32]; // Max payload size for RF24
};

/* Function Declarations */
bool rf_init();
bool rf_send(uint8_t to_id, const RFMessage &msg);
bool rf_receive(RFMessage &msg, unsigned long timeout_ms);

void rf_stop_listening();
void rf_start_listening();
void rf_set_rx_address(uint8_t id);
String rf_format_address(uint16_t node_id);
