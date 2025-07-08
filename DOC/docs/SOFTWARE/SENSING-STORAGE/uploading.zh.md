# 数据上传

由于是互联网项目，我们可以通过互联网将数据上传到服务器。由于我们引入了MQTT协议，所以我们可以通过MQTT协议将数据上传到服务器。但是注意，由于MQTT协议的限制，我们不能直接上传整个文件，而是需要将文件分成多个小块进行上传。此外，目前数据的上传是基于服务器端请求进行的，也就是说，节点会侦听服务器端的请求，当服务器端请求数据时，节点会将数据上传到服务器。其核心代码在sensing.cpp中。

!!! warning "注意"
    如果每个节点联网，那么采样文件可以直接采用上面所述方式上传到服务器。但是实际部署过程中发现，手机热点往往无法覆盖所有节点，所以目前数据还只是存储在SD卡上。主节点的数据因为其和mqtt服务器联网，可以直接上传到服务器。其他节点的数据则需要通过主节点进行转发，但是这牵扯到复杂的通信编程开发，且传输过程要远比采样时间长，本项目中暂时不实现。

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