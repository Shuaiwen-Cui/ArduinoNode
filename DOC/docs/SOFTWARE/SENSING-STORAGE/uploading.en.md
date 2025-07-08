# DATA UPLOADING

Since it is an Internet project, we can upload data to the server through the Internet. Since we have introduced the MQTT protocol, we can upload data to the server through the MQTT protocol. However, please note that due to the limitations of the MQTT protocol, we cannot directly upload the entire file, but need to divide the file into multiple small blocks for uploading. In addition, the current data upload is based on server-side requests, that is, the node will listen to the server-side request, and when the server-side requests data, the node will upload the data to the server. The core code is in sensing.cpp.

!!! warning "Note"
    If each node is connected to the Internet, then the sampling file can be uploaded to the server in the manner described above. However, it has been found that mobile hotspots often cannot cover all nodes in actual deployment, so currently data is only stored on the SD card. The data from the main node can be directly uploaded to the server because it is connected to the MQTT server. The data from other nodes needs to be forwarded through the main node, but this involves complex communication programming development, and the transmission process takes much longer than the sampling time, which is not implemented in this project for now.

```cpp

void sensing_retrieve_file()
{
    File file = SD.open(retrieval_filename, FILE_READ);
    if (!file)
    {
        Serial.print("[Error] File not found: ");
        Serial.println(retrieval_filename);
        return;
    }

    Serial.print("[Retrieval] Reading file: ");
    Serial.println(retrieval_filename);

    size_t total_size = file.size();
    size_t bytes_sent = 0;
    size_t chunk_size = 850;
    size_t chunk_index = 1;
    size_t chunk_total = (total_size + chunk_size - 1) / chunk_size;

    char prefix[32];
    snprintf(prefix, sizeof(prefix), "%s", retrieval_filename + 1); // Remove leading '/'
    char topic[64];

    while (file.available())
    {
        char buffer[851]; // chunk_size + 1 for null terminator
        size_t len = file.readBytes(buffer, chunk_size);
        buffer[len] = '\0';

        snprintf(topic, sizeof(topic), "%s[%d/%d]:", prefix, chunk_index, chunk_total);
        String payload = String(topic) + String(buffer);

        bool ok = mqtt_client.publish(MQTT_TOPIC_PUB, payload.c_str());
        if (ok)
        {
            bytes_sent += len;
            Serial.print("[MQTT] Sent chunk ");
            Serial.print(chunk_index);
            Serial.print(" / ");
            Serial.print(chunk_total);
            Serial.print(" (");
            Serial.print(bytes_sent);
            Serial.print(" / ");
            Serial.print(total_size);
            Serial.println(" bytes)");
        }
        else
        {
            Serial.print("[Error] Failed to send chunk ");
            Serial.println(chunk_index);
        }

        chunk_index++;

        mqtt_loop(); // keep MQTT alive
        delay(50);   // throttle
    }

    file.close();

    String done_msg = String(prefix) + "[done]";
    mqtt_client.publish(MQTT_TOPIC_PUB, done_msg.c_str());
    Serial.println("[MQTT] File upload completed.");

    node_status.node_flags.data_retrieval_requested = false;
    node_status.node_flags.data_retrieval_sent = true;
}

```