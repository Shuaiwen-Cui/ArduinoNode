#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"
#include "rf.hpp"
#include "rgbled.hpp"
#include "wifi.hpp"
#include "mqtt.hpp"

// For GATEWAY
void rf_command(const char *cmd);

// For LEAFNODE
void rf_handle();

