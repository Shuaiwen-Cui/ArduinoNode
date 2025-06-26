#pragma once
#include <Arduino.h>
#include "config.hpp"
#include "nodestate.hpp"

/*
 * Time synchronization header
 * 
 * Provides:
 * - NTP synchronization function
 * - RF-based online check and RTT estimation
 * - Stores RF latency (RTT/2) and online status for each node
 */

/* === RF Time Synchronization Configuration === */
#define RF_PING_PAYLOAD "PING"
#define RF_PONG_PAYLOAD "PONG"
#define RF_TIME_SYNC_HEADER "TIME_SYNC"
#define RF_TIME_ACK_HEADER "TIME_ACK"
#define RF_RESPONSE_WAIT_MS      300   // 300 ms wait per reply
#define RF_RETRY_PER_CYCLE         5   // Retry 5 times per send

extern int32_t node_rf_latency[NUM_NODES + 1];
extern bool node_online[NUM_NODES + 1];

bool sync_time_ntp();
bool sync_check_rf_online();
bool rf_time_sync();