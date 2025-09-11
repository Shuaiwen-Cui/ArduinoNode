# 编程

## 1 准备工作

### 1.1 安装 VSCode

<div class="grid cards" markdown>

-   :material-microsoft-visual-studio-code:{ .lg .middle } __VSCode__

    ---

    [:octicons-arrow-right-24: <a href="https://code.visualstudio.com/download" target="_blank"> 下载链接 </a>](#)

</div>

### 1.2 PlatformIO安装

![](platformio.png)

(1) 打开 VSCode 扩展管理器

(2) 搜索官方 PlatformIO IDE 扩展

(3) 安装 PlatformIO IDE。


### 1.3 Serial Monitor 安装

![](serial-monitor.png)

(1) 打开 VSCode 扩展管理器

(2) 搜索官方 Serial Monitor 扩展

(3) 安装 Serial Monitor 扩展。

### 1.4 下载项目代码

<div class="grid cards" markdown>

-   :material-file:{ .lg .middle } __GitHub__

    ---

    [:octicons-arrow-right-24: <a href="https://github.com/Shuaiwen-Cui/ArduinoNode/blob/main/CODE.zip" target="_blank"> 下载链接 </a>](#)

</div>

!!! tip
    直接点击下载按钮即可下载项目代码压缩包。

下载后，将文件解压到您选择的目录中。

## 2 编程

### 2.1 打开项目

(1) 打开 VSCode

(2) 点击左侧的文件图标，选择“打开文件夹”

(3) 选择解压后的项目文件夹打开


### 2.2 修改配置

这里需要将`src/config.hpp`文件中的以下内容修改为所需对应信息。打开`src/config.hpp`文件，找到以下代码段：

```cpp
#pragma once
#include <Arduino.h>

/* Node Information */
// #define GATEWAY          // for main node
#define LEAFNODE        // for sensor node

// #define NODE_ID 100      // GATEWAY should be 100
#define NODE_ID 1 // for LEAFNODE: 1, 2, 3, 4, 5, 6,
// #define NODE_ID 2
// #define NODE_ID 3
// #define NODE_ID 4
// #define NODE_ID 5
// #define NODE_ID 6

#define NUM_NODES 6 // Total number of nodes in the network

/* WiFi Credentials */
#define WIFI_SSID "Shaun's Iphone"
#define WIFI_PASSWORD "cshw0918"

/* MQTT Configurations */
// #define MQTT_CLIENT_ID      "GATEWAY"
#define MQTT_CLIENT_ID      "LEAFNODE1"
// #define MQTT_CLIENT_ID      "LEAFNODE2"
// #define MQTT_CLIENT_ID      "LEAFNODE3"
// #define MQTT_CLIENT_ID      "LEAFNODE4"
// #define MQTT_CLIENT_ID      "LEAFNODE5"
// #define MQTT_CLIENT_ID      "LEAFNODE6"

#define MQTT_BROKER_ADDRESS "8.222.194.160"
#define MQTT_BROKER_PORT    1883
#define MQTT_USERNAME       "ArduinoNode"
#define MQTT_PASSWORD       "Arduino123"
#define MQTT_TOPIC_PUB      "ArduinoNode/node"
#define MQTT_TOPIC_SUB      "ArduinoNode/server"

// Sensing Variables 
extern uint64_t sensing_scheduled_start_ms; // Scheduled sensing start time (Unix ms)
extern uint64_t sensing_scheduled_end_ms;   // Scheduled sensing end time (Unix ms)
extern uint32_t sensing_rate_hz;            // Sensing rate in Hz
extern uint32_t sensing_duration_s;         // Sensing duration in seconds
extern uint32_t default_sensing_rate_hz;   // Default sensing rate in Hz
extern uint32_t default_sensing_duration_s; // Default sensing duration in seconds
extern uint16_t parsed_freq;                // Parsed frequency from command
extern uint16_t parsed_duration;            // Parsed duration from command
extern float cali_scale_x; // Calibration scale for X-axis
extern float cali_scale_y; // Calibration scale for Y-axis
extern float cali_scale_z; // Calibration scale for Z-axis

/* Serial Configurations */
// #define DATA_PRINTOUT // Enable data printout to Serial

// === Function Declaration ===
void print_node_config();
```

接下来需要依次修改配置文件然后给每个节点烧录，需要先修改以下几处：

对于主节点（GATEWAY）：

- 请讲`#define GATEWAY`取消注释，并将`#define LEAFNODE`注释掉。

- 将`NODE_ID`设置为100，其他的`NODE_ID`注释掉。

- 将`MQTT_CLIENT_ID`设置为`GATEWAY`，其他的`MQTT_CLIENT_ID`注释掉。


对于传感节点（LEAFNODE）：

- 请将`#define LEAFNODE`取消注释，并将`#define GATEWAY`注释掉。

- 将`NODE_ID`设置为1-6中的一个数字，其他的`NODE_ID`注释掉。

- 将`MQTT_CLIENT_ID`设置为`LEAFNODE1`-`LEAFNODE6`中的一个，其他的`MQTT_CLIENT_ID`注释掉。

总节点数：

- 请将`NUM_NODES`设置为网络中的总节点数，例如6。


### 2.3 编译和上传代码

按照以下步骤编译和上传代码：

![](icons.jpg)

1. 确保已经通过Type-C连接将开发板连接到电脑。
2. 在VSCode底部Platformio的操作图标中，选择“编译”按钮进行代码编译。
3. 编译完成后，选择“上传”按钮将代码上传到开发板。

### 2.4 查看串口输出

您可以通过Platformio自带的串口监视器查看开发板的输出。点击底部的“串口监视器”按钮，选择正确的串口号（通常是COM3或类似名称），设定波特率为115200，然后点击“连接”按钮进行查看。

!!! warning
    串口同一时间只能被一个程序使用，请确保没有其他程序占用串口。注意烧录的时候，先把串口通讯关掉，否则烧录可能会失败。

## 3 测试

在烧录完成后，您可以通过RGBLED颜色和串口输出判断是否正常工作。

!!! tip
    如果RGBLED颜色始终为白色，说明初始化过程当中出现了问题，请检查硬件链接。天蓝色状态为RF通讯，说明在等待与主节点进行通讯，需要配合主节点的信号进行测试。绿色说明已经完成初始化，进入了IDLE状态，等待主节点的指令。黄色是采集前的准备状态，紫色是正在采集数据状态。红色是错误状态，可能是由于传感器初始化失败或者其他问题导致的。