# SD卡模块

!!! info "SD卡模块"
    SD卡模块是一个用于存储数据的外部存储设备。它可以通过SPI接口与Arduino进行通信。SD卡模块通常用于存储日志数据、配置文件或其他需要持久化的数据。

![](sdcard.jpg){width=70%}

## 硬件连接

![](sdcard-wiring.png)

| Arduino 引脚 | SD卡模块引脚 |
|-------------|--------------------|
| 5V          | VCC                |
| GND         | GND                |
| D10         | CS                 |
| D11         | MOSI               |
| D12         | MISO               |
| D13         | SCK                |

!!! tip "注意"
    SD卡容量推荐在32GB以下，使用FAT32格式化。大容量SD卡可能会导致兼容性问题。