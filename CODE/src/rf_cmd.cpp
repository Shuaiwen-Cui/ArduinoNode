#include "rf_cmd.hpp"

void rf_command(const char *cmd)
{
    RFMessage msg;
    msg.from_id = NODE_ID;
    strncpy(msg.payload, cmd, sizeof(msg.payload));

    for (uint8_t target_id = 1; target_id <= NUM_NODES; ++target_id)
    {
        msg.to_id = target_id;

        Serial.print("[GATEWAY] Sending RF Command to Node ");
        Serial.print(target_id);
        Serial.print(": ");
        Serial.println(msg.payload);

        rf_stop_listening();
        bool success = rf_send(msg.to_id, msg);
        if (success)
        {
            Serial.println("[GATEWAY] Command sent successfully.");
        }
        else
        {
            Serial.println("[GATEWAY] Failed to send command.");
        }
        rf_start_listening();
    }
}

void send_command_with_retry(const char *cmd)
{
    for (int attempt = 0; attempt < RF_CMD_RETRY; ++attempt)
    {
        rf_command(cmd);
        delay(RF_CMD_WAIT_MS);
    }
}

void rf_handle()
{
    RFMessage msg;

    if (rf_receive(msg, 200)) // 200ms timeout
    {
        if (msg.to_id != NODE_ID)
            return;

        Serial.print("[RF_COMMUNICATION] Message received from Node ");
        Serial.print(msg.from_id);
        Serial.print(": ");
        Serial.println(msg.payload);

        // === CMD_REBOOT ===
        if (strcmp(msg.payload, "CMD_REBOOT") == 0)
        {
            Serial.println("[LEAFNODE] Reboot command received.");
            node_status.node_flags.reboot_required_leafnode = true;
            node_status.set_state(NodeState::BOOT);
            rgbled_set_by_state(NodeState::BOOT);
        }

        // === CMD_RF_SYNC ===
        else if (strcmp(msg.payload, "CMD_RF_SYNC") == 0)
        {
            Serial.println("[LEAFNODE] RF Sync command received.");
            node_status.node_flags.time_rf_required = true;
            node_status.set_state(NodeState::RF_COMMUNICATING);
            rgbled_set_by_state(NodeState::RF_COMMUNICATING);
        }

        // === Sensing Schedule Command ===
        else if (strncmp(msg.payload, "S_", 2) == 0)
        {
            Serial.println("[LEAFNODE] Sensing command received.");

            // Step 1: Extract 12-digit time
            char datetime[13] = {0};
            strncpy(datetime, msg.payload + 2, 12);

            // Step 2: Find first and second underscore after time part
            const char *ptr = msg.payload + 14;
            const char *first_underscore = strchr(ptr, '_');
            if (!first_underscore)
            {
                Serial.println("[LEAFNODE] Invalid sensing command format: missing first underscore.");
                return;
            }

            const char *second_underscore = strchr(first_underscore + 1, '_');
            if (!second_underscore)
            {
                Serial.println("[LEAFNODE] Invalid sensing command format: missing second underscore.");
                return;
            }

            // Step 3: Extract substrings for rate and duration
            char rate_buf[6] = {0};
            char dur_buf[6] = {0};

            size_t rate_len = second_underscore - (first_underscore + 1);
            size_t dur_len = strlen(second_underscore + 1);

            if (rate_len >= sizeof(rate_buf) || dur_len >= sizeof(dur_buf))
            {
                Serial.println("[LEAFNODE] Rate or duration value too long.");
                return;
            }

            strncpy(rate_buf, first_underscore + 1, rate_len);
            strncpy(dur_buf, second_underscore + 1, dur_len);

            int rate = atoi(rate_buf);
            int dur = atoi(dur_buf);

            CalendarTime SensingSchedule;
            SensingSchedule = YYMMDDHHMMSS2Calendar(datetime);

            parsed_freq = rate;
            sensing_rate_hz = parsed_freq;
            parsed_duration = dur;
            sensing_duration_s = parsed_duration;

            sensing_scheduled_start_ms = unix_from_calendar_milliseconds(SensingSchedule);
            sensing_scheduled_end_ms = sensing_scheduled_start_ms + dur * 1000;

            node_status.node_flags.sensing_scheduled = true; // very important!

            // Debug print
            Serial.print("[LEAFNODE] Parsed Time: ");
            Serial.print(SensingSchedule.year);
            Serial.print("-");
            Serial.print(SensingSchedule.month);
            Serial.print("-");
            Serial.print(SensingSchedule.day);
            Serial.print(" ");
            Serial.print(SensingSchedule.hour);
            Serial.print(":");
            Serial.print(SensingSchedule.minute);
            Serial.print(":");
            Serial.println(SensingSchedule.second);

            Serial.print("[LEAFNODE] Parsed Rate = ");
            Serial.print(parsed_freq);
            Serial.print(" Hz, Duration = ");
            Serial.print(parsed_duration);
            Serial.println(" sec");

            Serial.print("[LEAFNODE] Scheduled Start Time (ms): ");
            Serial.println(sensing_scheduled_start_ms);

            Serial.print("[LEAFNODE] Scheduled Sampling Rate: ");
            Serial.print(sensing_rate_hz);

            Serial.print(" Hz, Duration: ");
            Serial.print(sensing_duration_s);
            Serial.println(" sec");
        }
        // === Unknown Command ===
        else
        {
            Serial.println("[RF_COMMUNICATION] Unknown command.");
        }
    }
}
