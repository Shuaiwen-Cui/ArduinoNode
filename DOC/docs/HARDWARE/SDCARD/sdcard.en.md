# SD CARD MODULE

!!! info "SD Card Module"
    The SD card module is an external storage device used for data storage. It communicates with Arduino via the SPI interface. The SD card module is typically used to store log data, configuration files, or other data that needs to be persistent.

![](sdcard.jpg){width=70%}

## Hardware Connections

![](sdcard-wiring.png)

| Arduino Pin | SD Card Module Pin |
|-------------|--------------------|
| 5V          | VCC                |
| GND         | GND                |
| D10         | CS                 |
| D11         | MOSI               |
| D12         | MISO               |
| D13         | SCK                |

!!! tip "Note"
    It is recommended to use SD cards with a capacity of 32GB or less, formatted in FAT32. Larger capacity SD cards may cause compatibility issues.