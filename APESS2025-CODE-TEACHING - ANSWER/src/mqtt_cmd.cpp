#include "mqtt_cmd.hpp"
#include "rgbled.hpp"
#include "time.hpp"
#include "timesync.hpp"

// Parsed Command Variables
char cmd_sensing_raw[128];

// Callback when subscribed message is received
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("[COMMUNICATION] <MQTT> Message received [");
    Serial.print(topic);
    Serial.print("]: ");

    char message[length + 1];
    for (unsigned int i = 0; i < length; ++i)
    {
        message[i] = (char)payload[i];
        Serial.print(message[i]);
    }
    message[length] = '\0';
    Serial.println();

    // Clean trailing \r or \n
    while (length > 0 && (message[length - 1] == '\r' || message[length - 1] == '\n'))
    {
        message[--length] = '\0';
    }

    String msg_str(message);

    if (msg_str == "CMD_NTP")
    {
        node_status.node_flags.gateway_ntp_required = true;
        node_status.node_flags.leafnode_ntp_required = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_NTP received.");

        // switch to COMMUNICATING state
        node_status.set_state(NodeState::WIFI_COMMUNICATING);
        rgbled_set_by_state(NodeState::WIFI_COMMUNICATING); // Set LED to blue during NTP sync
    }
    // else if (msg_str == "CMD_GATEWAY_NTP")
    // {
    //     node_status.node_flags.gateway_ntp_required = true;
    //     Serial.println("[COMMUNICATION] <CMD> CMD_GATEWAY_NTP received.");

    //     // switch to COMMUNICATING state
    //     node_status.set_state(NodeState::WIFI_COMMUNICATING);
    //     rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
    // }
    // else if (msg_str == "CMD_LEAFNODE_NTP")
    // {
    //     node_status.node_flags.leafnode_ntp_required = true;
    //     Serial.println("[COMMUNICATION] <CMD> CMD_LEAFNODE_NTP received.");

    //     // switch to COMMUNICATING state
    //     node_status.set_state(NodeState::WIFI_COMMUNICATING);
    //     rgbled_set_all(CRGB::Blue); // Set LED to blue during NTP sync
    // }
    else if (msg_str == "CMD_RF_SYNC")
    {
        node_status.node_flags.time_rf_required = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_RF_SYNC received.");
    }
    else if (msg_str == "CMD_SN")
    {
        Serial.println("[COMMUNICATION] <CMD> CMD_SN received.");

        node_status.node_flags.time_rf_required = true;

        // Get current system time in milliseconds
        uint64_t now_unix_ms = Time.get_time();
        uint64_t now_unix_ms_rounded = (now_unix_ms / 1000) * 1000; // Round down to nearest second

        // Schedule sensing: start after TIME_SYNC_RESERVED_TIME
        
        sensing_duration_s = default_sensing_duration_s; // Use default duration
        sensing_rate_hz = default_sensing_rate_hz;       // Use default rate

        parsed_freq = default_sensing_rate_hz;    // global variable at config.hpp
        parsed_duration = default_sensing_duration_s; // global variable at config.hpp

        sensing_scheduled_start_ms = now_unix_ms_rounded + TIME_SYNC_RESERVED_TIME;
        sensing_scheduled_end_ms = sensing_scheduled_start_ms + (sensing_duration_s * 1000);

        // Set sensing flags
        node_status.node_flags.sensing_requested = true;
        node_status.node_flags.sensing_scheduled = true;

        // Convert scheduled start time to human-readable calendar format
        CalendarTime start_ct = calendar_from_unix_milliseconds(sensing_scheduled_start_ms);

        // Print schedule details to Serial
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "[MQTT] CMD_SN: Sensing scheduled at %04d-%02d-%02d %02d:%02d:%02d | Freq = %d Hz, Duration = %d s",
                 start_ct.year, start_ct.month, start_ct.day,
                 start_ct.hour, start_ct.minute, start_ct.second,
                 sensing_rate_hz, sensing_duration_s);
        Serial.println(buf);

        // Optionally publish feedback to MQTT broker
        mqtt_client.publish(MQTT_TOPIC_PUB, "CMD_SN: Sensing scheduled using default parameters.");
    }
    else if (msg_str.startsWith("CMD_SFN_"))
    {
        Serial.println("[COMMUNICATION] <CMD> CMD_SFN received.");

        node_status.node_flags.time_rf_required = true;

        int delay_sec, freq, duration;
        int matched = sscanf(message, "CMD_SFN_%d_%dHz_%ds", &delay_sec, &freq, &duration);

        if (matched == 3)
        {
            if (delay_sec * 1000 < TIME_SYNC_RESERVED_TIME)
            {
                Serial.println("[MQTT] CMD_SFN rejected: insufficient delay for time synchronization.");
                mqtt_client.publish(MQTT_TOPIC_PUB, "CMD_SFN ignored: delay too short for time sync.");
                rgbled_set_all(CRGB::Red); // Visual error indication
                delay(3000);
                if (node_status.get_state() == NodeState::IDLE)
                    rgbled_set_by_state(NodeState::IDLE);
            }
            else
            {
                uint64_t now_unix_ms = Time.get_time();
                uint64_t now_unix_ms_rounded = (now_unix_ms / 1000) * 1000; // Round down to nearest second
                sensing_scheduled_start_ms = now_unix_ms_rounded + (uint64_t)delay_sec * 1000;
                sensing_scheduled_end_ms = sensing_scheduled_start_ms + (uint64_t)duration * 1000;
                sensing_rate_hz = freq;
                sensing_duration_s = duration;

                parsed_freq = freq;    // global variable at config.hpp
                parsed_duration = duration; // global variable at config.hpp

                node_status.node_flags.sensing_requested = true;
                node_status.node_flags.sensing_scheduled = true;

                CalendarTime start_ct = calendar_from_unix_milliseconds(sensing_scheduled_start_ms);

                char buf[128];
                snprintf(buf, sizeof(buf),
                         "[MQTT] CMD_SFN: Sensing scheduled at %04d-%02d-%02d %02d:%02d:%02d | Freq = %d Hz, Duration = %d s",
                         start_ct.year, start_ct.month, start_ct.day,
                         start_ct.hour, start_ct.minute, start_ct.second,
                         sensing_rate_hz, sensing_duration_s);
                Serial.println(buf);

                mqtt_client.publish(MQTT_TOPIC_PUB, "CMD_SFN: Sensing successfully scheduled.");
            }
        }
        else
        {
            Serial.println("[MQTT] CMD_SFN format error.");
            mqtt_client.publish(MQTT_TOPIC_PUB, "CMD_SFN ignored: invalid format.");
            rgbled_set_all(CRGB::Red);
            delay(3000);
            if (node_status.get_state() == NodeState::IDLE)
                rgbled_set_by_state(NodeState::IDLE);
        }
    }
    else if (msg_str.startsWith("CMD_SENSING_"))
    {
        node_status.node_flags.time_rf_required = true;

        strncpy(cmd_sensing_raw, message, sizeof(cmd_sensing_raw) - 1);
        cmd_sensing_raw[sizeof(cmd_sensing_raw) - 1] = '\0';
        node_status.node_flags.sensing_requested = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_SENSING received.");

        int y, mo, d, h, mi, s;
        int rate, dur;
        int matched = sscanf(message,
                             "CMD_SENSING_%d-%d-%d_%d:%d:%d_%dHz_%ds",
                             &y, &mo, &d, &h, &mi, &s, &rate, &dur);
        int ms_value = 0;

        if (matched == 8)
        {
            CalendarTime ParseTime;
            ParseTime.year = y;
            ParseTime.month = mo;
            ParseTime.day = d;
            ParseTime.hour = h;
            ParseTime.minute = mi;
            ParseTime.second = s;
            ParseTime.ms = 0;

            parsed_freq = rate;    // global variable at config.hpp
            parsed_duration = dur; // global variable at config.hpp

            uint64_t now_unix_ms = Time.get_time();
            uint64_t parsed_temp_sensing_start_ms = unix_from_calendar_milliseconds(ParseTime);

            if (now_unix_ms > parsed_temp_sensing_start_ms)
            {
                Serial.println("[MQTT] Sensing start time is in the past, ignoring command.");
                node_status.node_flags.sensing_requested = false;
                node_status.node_flags.sensing_scheduled = false;

                // feedback to the mqtt broker
                mqtt_client.publish(MQTT_TOPIC_PUB, "Sensing command ignored: start time is in the past!");

                rgbled_set_all(CRGB::Red); // Set LED to red to indicate error
                delay(3000);               // Wait for 2 seconds to indicate error
                if (node_status.get_state() == NodeState::IDLE)
                {
                    rgbled_set_by_state(NodeState::IDLE); // Reset LED to IDLE state
                }
            }
            else if (parsed_temp_sensing_start_ms < now_unix_ms + TIME_SYNC_RESERVED_TIME)
            {
                Serial.println("[ERROR] Not enough time for time synchronization, must larger than TIME_SYNC_RESERVED_TIME (by default 60 seconds), ignoring command.");
                node_status.node_flags.sensing_requested = false;
                node_status.node_flags.sensing_scheduled = false;

                // feedback to the mqtt broker
                mqtt_client.publish(MQTT_TOPIC_PUB, "Sensing command ignored: not enough time for time synchronization!");

                rgbled_set_all(CRGB::Red); // Set LED to red to indicate error
                delay(3000);               // Wait for 2 seconds to indicate error
                if (node_status.get_state() == NodeState::IDLE)
                {
                    rgbled_set_by_state(NodeState::IDLE); // Reset LED to IDLE state
                }
            }
            else
            {
                Serial.println("[MQTT] Scheduling sensing...");
                sensing_scheduled_start_ms = parsed_temp_sensing_start_ms;
                sensing_scheduled_end_ms = sensing_scheduled_start_ms + (parsed_duration * 1000);
                sensing_rate_hz = parsed_freq;
                sensing_duration_s = parsed_duration;

                node_status.node_flags.sensing_scheduled = true;

                char buf[128];
                snprintf(buf, sizeof(buf), "[MQTT] Sensing scheduled, sampling at %d Hz for %d seconds, starting at %04d-%02d-%02d %02d:%02d:%02d",
                         sensing_rate_hz, sensing_duration_s,
                         ParseTime.year, ParseTime.month, ParseTime.day,
                         ParseTime.hour, ParseTime.minute, ParseTime.second);
                Serial.println(buf);
            }
        }
        else
        {
            Serial.println("[MQTT] Failed to parse CMD_SENSING command.");
            node_status.node_flags.sensing_requested = false;
        }
    }
    else if (msg_str.startsWith("CMD_RETRIEVAL_"))
    {
        const char *filename_part = message + 14;
        snprintf(retrieval_filename, sizeof(retrieval_filename), "/%s.txt", filename_part);
        node_status.node_flags.data_retrieval_requested = true;
        node_status.node_flags.data_retrieval_sent = false; // Reset sent flag for new retrieval

        Serial.print("[COMMUNICATION] <CMD> CMD_RETRIEVAL received: ");
        Serial.println(retrieval_filename);

        // switch to COMMUNICATING state
        node_status.set_state(NodeState::WIFI_COMMUNICATING);
        rgbled_set_all(CRGB::Blue); // Set LED to blue during data retrieval
    }
    else if (msg_str == "CMD_REBOOT")
    {
        node_status.node_flags.reboot_required_gateway = true;
        node_status.node_flags.reboot_required_leafnode = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_REBOOT received.");
    }
    else if (msg_str == "CMD_GATEWAY_REBOOT")
    {
        node_status.node_flags.reboot_required_gateway = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_GATEWAY_REBOOT received.");
    }
    else if (msg_str == "CMD_LEAFNODE_REBOOT")
    {
        node_status.node_flags.reboot_required_leafnode = true;
        Serial.println("[COMMUNICATION] <CMD> CMD_LEAFNODE_REBOOT received.");
    }
    else
    {
        Serial.println("[COMMUNICATION] <CMD> Unknown command.");
    }
}
