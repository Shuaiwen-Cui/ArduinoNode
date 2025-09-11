#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include "rf.hpp"
#include "rgbled.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "time.hpp"

#define RF_CMD_RETRY        3     
#define RF_CMD_WAIT_MS      100   

// For GATEWAY
void rf_command(const char *cmd);
void send_command_with_retry(const char *cmd);

// For LEAFNODE
void rf_handle();

