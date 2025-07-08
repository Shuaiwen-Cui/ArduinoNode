# ARCHITECTURE

In this project, the dependency graph of the software architecture is as follows:

```mermaid
graph LR

%% === Module Definitions ===
config["config.hpp/.cpp"]
nodestate["nodestate.hpp/.cpp"]
time["time.hpp/.cpp"]
mpu["mpu6050.hpp/.cpp"]
sd["sdcard.hpp/.cpp"]
rgb["rgbled.hpp/.cpp"]
log["logging.hpp/.cpp"]
mqtt["mqtt.hpp/.cpp"]
sensing["sensing.hpp/.cpp"]
sync["timesync.hpp/.cpp"]
wifi["wifi.hpp/.cpp"]
rf["rf.hpp/.cpp"]
rf_cmd["rf_cmd.hpp/.cpp"]
main["main.cpp"]

%% === Module Relationships ===
config --> mqtt
config --> sensing
config --> rf
config --> rf_cmd
config --> sync
config --> main

nodestate --> mqtt
nodestate --> sensing
nodestate --> rf_cmd
nodestate --> sync
nodestate --> main

time --> sensing
time --> sync
time --> main

mpu --> sensing
mpu --> main

sd --> sensing
sd --> log
sd --> main

rgb --> mqtt
rgb --> sensing
rgb --> rf_cmd
rgb --> main

log --> sensing

mqtt --> sensing
mqtt --> main

sensing --> main

sync --> main

wifi --> main

rf --> rf_cmd
rf --> sync
rf --> main

rf_cmd --> main


```