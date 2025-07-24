#include <Arduino.h>
#include "nodestate.hpp"

// Define the global instance
NodeStatusManager node_status;

// Constructor implementation
NodeStatusManager::NodeStatusManager()
{
    node_state = NodeState::BOOT;

    // All flags initialized to false by default via struct default values
}

// Set current node state
void NodeStatusManager::set_state(NodeState new_state)
{
    node_state = new_state;
}

// Get current node state
NodeState NodeStatusManager::get_state() const
{
    return node_state;
}

// Print current state and flags
void NodeStatusManager::print_state() const
{
    Serial.print("[STATUS] Current state: ");
    switch (node_state)
    {
    case NodeState::BOOT:
        Serial.println("BOOT");
        break;
    case NodeState::IDLE:
        Serial.println("IDLE");
        break;
    case NodeState::PREPARING:
        Serial.println("PREPARING");
        break;
    case NodeState::SAMPLING:
        Serial.println("SAMPLING");
        break;
    case NodeState::RF_COMMUNICATING:
        Serial.println("RF_COMMUNICATING");
        break;
    case NodeState::WIFI_COMMUNICATING:
        Serial.println("WIFI_COMMUNICATING");
        break;
    case NodeState::ERROR:
        Serial.println("ERROR");
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }

    Serial.println("<NodeFlags> Initialization:");
    Serial.print("  Serial Ready: ");
    Serial.println(node_flags.serial_ready ? "Yes" : "No");
    Serial.print("  LED Ready:    ");
    Serial.println(node_flags.led_ready ? "Yes" : "No");
    Serial.print("  IMU Ready:    ");
    Serial.println(node_flags.imu_ready ? "Yes" : "No");
    Serial.print("  RF Ready:     ");
    Serial.println(node_flags.rf_ready ? "Yes" : "No");
    Serial.print("  SD Ready:     ");
    Serial.println(node_flags.sd_ready ? "Yes" : "No");

    Serial.println("<NodeFlags> Connection:");
    Serial.print("  WiFi:         ");
    Serial.println(node_flags.wifi_connected ? "Yes" : "No");
    Serial.print("  MQTT:         ");
    Serial.println(node_flags.mqtt_connected ? "Yes" : "No");

    Serial.println("<NodeFlags> Time Sync:");
    Serial.print("  NTP:          ");
    Serial.println(node_flags.time_ntp_synced ? "Yes" : "No");
    Serial.print("  RF:           ");
    Serial.println(node_flags.time_rf_synced ? "Yes" : "No");

    Serial.println();
}
