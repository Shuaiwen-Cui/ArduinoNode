#pragma once

// === Mutually exclusive node states ===
enum class NodeState
{
    BOOT,
    IDLE,               // routine operation & monitoring
    PREPARING,          // no routine operation, preparing for sensing
    SAMPLING,           // actively sampling data
    RF_COMMUNICATING,   // communicating with other nodes via RF
    WIFI_COMMUNICATING, // communicating with server via WiFi
    ERROR               // error state
};

// === Non-mutually-exclusive status flags ===
struct NodeFlags
{
    // Reboot Flags
    bool reboot_required_gateway = false;  // Reboot command received for gateway
    bool reboot_required_leafnode = false; // Reboot command received for leaf node

    // Initialization Flags
    bool serial_ready = false; // Serial communication ready status
    bool led_ready = false;    // LED ready status
    bool imu_ready = false;    // IMU sensor ready status
    bool rf_ready = false;     // RF communication ready status
    bool sd_ready = false;     // SD card ready status

    // Key Connection Flags
    bool wifi_connected = false; // WiFi connection status
    bool mqtt_connected = false; // MQTT connection status

    // Time Synchronization Flags
    bool gateway_ntp_required = false;  // Gateway NTP required status
    bool leafnode_ntp_required = false; // Leaf node NTP required status
    bool time_ntp_synced = false;       // NTP time synchronization status
    bool time_rf_required = false;      // RF time sync required status
    bool time_rf_synced = false;        // RF time synchronization status

    // Sensing Flags
    bool sensing_requested = false; // Sensing command received status
    bool sensing_scheduled = false; // Sensing schedule status
    bool sensing_active = false;    // Sensing activity status

    // Data Logging Flags
    bool data_retrieval_requested = false; // Data retrieval request status
    bool data_retrieval_sent = true;       // Data retrieval sent status, by default true, meaning already sent
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
