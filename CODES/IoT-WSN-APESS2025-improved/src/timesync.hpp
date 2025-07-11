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
#define RF_TIME_SYNC_HEADER "TSYNC"
#define RF_TIME_ACK_HEADER "TACK"
#define RF_RESPONSE_WAIT_MS      300   // 300 ms wait per reply
#define RF_RETRY_PER_CYCLE         5   // Retry 5 times per send

#define PONG_REPETITIONS 3
#define PONG_INTERVAL_MS 3000
#define RF_RESPONSE_WINDOW_MS (PONG_REPETITIONS * PONG_INTERVAL_MS + 500)

#define DRIFT_SYNC_SAMPLES       5
#define DRIFT_SYNC_INTERVAL_MS   5000
#define DRIFT_SYNC_TAG           "TSYNC_DRIFT"
#define RF_ACK_TIMEOUT_MS        200


extern bool node_online[NUM_NODES + 1];
extern int32_t node_rf_latency[NUM_NODES + 1];
extern float node_drift_ratio[NUM_NODES + 1];

bool sync_time_ntp();
bool check_rf_online_and_latency();
bool rf_sync_drift();
bool rf_time_sync();