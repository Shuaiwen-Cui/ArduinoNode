#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// === Command definitions ===
#define RF_CMD_NTP              "CMD_NTP"
#define RF_CMD_GATEWAY_NTP      "CMD_GATEWAY_NTP"
#define RF_CMD_LEAFNODE_NTP     "CMD_LEAFNODE_NTP"
#define RF_CMD_REBOOT           "CMD_REBOOT"
#define RF_CMD_GATEWAY_REBOOT   "CMD_GATEWAY_REBOOT"
#define RF_CMD_LEAFNODE_REBOOT  "CMD_LEAFNODE_REBOOT"
#define RF_CMD_SAMPLING_PREFIX  "S_"
#define RF_CMD_ACK_PREFIX       "ACK_"

// === Reliable send options ===
#define RF_MAX_RETRIES          5
#define RF_ACK_TIMEOUT_MS       200

// === Functions ===
bool rf_send_command(uint8_t to_id, const char *cmd);
bool rf_send_sampling_command(uint8_t to_id, const char *datetime, uint16_t rate_hz, uint16_t duration_s);
bool rf_send_command_reliable(uint8_t to_id, const char *cmd);
void rf_loop_handle_command();
bool should_run_rf_loop();
