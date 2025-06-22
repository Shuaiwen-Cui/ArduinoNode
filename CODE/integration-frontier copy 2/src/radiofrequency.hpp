#pragma once

#include <Arduino.h>
#include <RF24.h>

// === Initialization ===
void rf_init();

// === Addressing ===
void rf_set_peer_address(uint16_t node_id);  // For GATEWAY

// === Communication ===
bool rf_send(const void *data, size_t len);      // For LEAFNODE
bool rf_receive(void *data, size_t len);         // For GATEWAY
