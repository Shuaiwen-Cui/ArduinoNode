#include "rf_cmd.hpp"
#include "config.hpp"
#include "nodestate.hpp"
#include "rgbled.hpp"
#include "time.hpp"
#include <string.h>
#include "sensing.hpp"
#include "rf.hpp"

// === Master node: send plain command
bool rf_send_command(uint8_t to_id, const char *cmd)
{
    RFMessage msg;
    msg.from_id = NODE_ID;
    msg.to_id = to_id;
    strncpy(msg.payload, cmd, sizeof(msg.payload));
    msg.payload[sizeof(msg.payload) - 1] = '\0';
    msg.timestamp_ms = millis();
    return rf_send(to_id, msg);
}

// === Master node: send formatted sampling command like "S_250309120104_100_030"
bool rf_send_sampling_command(uint8_t to_id, const char *datetime, uint16_t rate_hz, uint16_t duration_s)
{
    char cmd[23];
    snprintf(cmd, sizeof(cmd), "S_%s_%03u_%03u", datetime, rate_hz, duration_s);
    return rf_send_command(to_id, cmd);
}

// === Master node: send command with ACK verification
bool rf_send_command_reliable(uint8_t to_id, const char *cmd)
{
    for (int attempt = 0; attempt < RF_MAX_RETRIES; ++attempt)
    {
        Serial.print("[RF] Sending command (attempt ");
        Serial.print(attempt + 1);
        Serial.println("):");
        Serial.println(cmd);

        rf_send_command(to_id, cmd);

        unsigned long start = millis();
        while (millis() - start < RF_ACK_TIMEOUT_MS)
        {
            RFMessage ack_msg;
            if (rf_receive(ack_msg, 20))
            {
                if (ack_msg.from_id == to_id && strncmp(ack_msg.payload, RF_CMD_ACK_PREFIX, 4) == 0)
                {
                    Serial.println("[RF] ACK received.");
                    return true;
                }
            }
        }
    }

    Serial.println("[RF] Failed to receive ACK.");
    return false;
}

// === Leaf node: handle incoming command
void rf_loop_handle_command()
{
    RFMessage msg;

    if (rf_receive(msg, 50))
    {
        if (msg.to_id != NODE_ID && msg.to_id != 0xFF)
            return;

        Serial.print("[RF] Command received: ");
        Serial.println(msg.payload);

        String command = String(msg.payload);

        // === Acknowledge command ===
        if (msg.from_id != NODE_ID)  // Avoid ACK loop
        {
            RFMessage ack;
            ack.from_id = NODE_ID;
            ack.to_id = msg.from_id;
            snprintf(ack.payload, sizeof(ack.payload), "ACK_%s", msg.payload);
            ack.timestamp_ms = millis();
            rf_send(msg.from_id, ack);
        }

        // === Command parsing ===
        if (command == RF_CMD_NTP)
        {
            node_status.node_flags.gateway_ntp_required = true;
            node_status.node_flags.leafnode_ntp_required = true;
            Serial.println("[RF] <CMD> CMD_NTP received.");
        }
        else if (command == RF_CMD_GATEWAY_NTP)
        {
            node_status.node_flags.gateway_ntp_required = true;
            Serial.println("[RF] <CMD> CMD_GATEWAY_NTP received.");
        }
        else if (command == RF_CMD_LEAFNODE_NTP)
        {
            node_status.node_flags.leafnode_ntp_required = true;
            Serial.println("[RF] <CMD> CMD_LEAFNODE_NTP received.");
        }
        else if (command == RF_CMD_REBOOT)
        {
            node_status.node_flags.reboot_required_gateway = true;
            node_status.node_flags.reboot_required_leafnode = true;
            Serial.println("[RF] <CMD> CMD_REBOOT received.");
        }
        else if (command == RF_CMD_GATEWAY_REBOOT)
        {
            node_status.node_flags.reboot_required_gateway = true;
            Serial.println("[RF] <CMD> CMD_GATEWAY_REBOOT received.");
        }
        else if (command == RF_CMD_LEAFNODE_REBOOT)
        {
            node_status.node_flags.reboot_required_leafnode = true;
            Serial.println("[RF] <CMD> CMD_LEAFNODE_REBOOT received.");
        }
        else if (strncmp(msg.payload, RF_CMD_SAMPLING_PREFIX, 2) == 0)
        {
            char buf[23];
            strncpy(buf, msg.payload, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';

            char *datetime_str = strtok(buf + 2, "_");
            char *rate_str = strtok(NULL, "_");
            char *duration_str = strtok(NULL, "_");

            if (!datetime_str || !rate_str || !duration_str)
            {
                Serial.println("[RF] <CMD> Invalid sampling command format.");
                return;
            }

            if (!SensingSchedule.set_from_string_YYMMDDHHMMSS(datetime_str))
            {
                Serial.println("[RF] <CMD> Failed to parse datetime string.");
                return;
            }

            parsed_freq = atoi(rate_str);
            parsed_duration = atoi(duration_str);

            sensing_scheduled_start_ms = SensingSchedule.compute_ms_from_calendar();
            SensingSchedule.unix_ms = sensing_scheduled_start_ms;
            SensingSchedule.unix_epoch = sensing_scheduled_start_ms / 1000;
            SensingSchedule.set_calendar();

            sensing_scheduled_end_ms = sensing_scheduled_start_ms + (parsed_duration * 1000);
            sensing_rate_hz = parsed_freq;
            sensing_duration_s = parsed_duration;

            node_status.node_flags.sensing_requested = true;
            node_status.node_flags.sensing_scheduled = true;

            char buf_msg[128];
            snprintf(buf_msg, sizeof(buf_msg),
                     "[RF] Sensing scheduled, %d Hz for %d sec, start at %04d-%02d-%02d %02d:%02d:%02d",
                     parsed_freq, parsed_duration,
                     SensingSchedule.year, SensingSchedule.month, SensingSchedule.day,
                     SensingSchedule.hour, SensingSchedule.minute, SensingSchedule.second);
            Serial.println(buf_msg);
        }
        else
        {
            Serial.println("[RF] <CMD> Unknown command.");
        }
    }
}

// === Run RF loop every N ms to reduce CPU load
bool should_run_rf_loop()
{
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last >= 200)
    {
        last = now;
        return true;
    }
    return false;
}
