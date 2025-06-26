#pragma once

#include <Arduino.h>
#include <RF24.h>
#include "config.hpp"

// Gateway test function (send PING to all leaf nodes)
void rf_test_gateway();

// Leaf test function (wait for PING and reply PONG)
void rf_test_leaf();
