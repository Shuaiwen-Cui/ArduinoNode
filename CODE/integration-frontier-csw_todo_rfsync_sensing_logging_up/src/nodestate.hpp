#pragma once

// === Mutually exclusive node states ===
enum class NodeState
{
    BOOT,
    IDLE,
    SAMPLING,
    COMMUNICATING,
    ERROR
};

// === Non-mutually-exclusive status flags ===
struct NodeFlags
{
    // Initialization Flags
    bool serial_ready = false;    // Serial communication ready status
    bool led_ready = false;       // LED ready status
    bool imu_ready = false;       // IMU sensor ready status
    bool rf_ready = false;        // RF communication ready status
    bool sd_ready = false;         // SD card ready status

    // Key Connection Flags
    bool wifi_connected = false;   // WiFi connection status
    bool mqtt_connected = false;   // MQTT connection status

    // Time Synchronization Flags
    bool time_ntp_synced = false;   // NTP time synchronization status
    bool time_rf_synced = false;   // RF time synchronization status
};

// === State Manager ===
class NodeStatusManager
{
public:
    // Current state & flags
    NodeState node_state;
    NodeFlags node_flags;

    // Constructor
    NodeStatusManager();

    // State setters
    void set_state(NodeState new_state);
    NodeState get_state() const;

    // Debug print
    void print_state() const;
};

// Global instance
extern NodeStatusManager node_status;
