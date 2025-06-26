# 反馈

本项目中的节点状态反馈是通过外接的RGB LED灯来实现的。以下是相关代码：

**rgbled.hpp**

```cpp
#pragma once

#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 4
#define LED_PIN 7

extern CRGB leds[NUM_LEDS];

void rgbled_init();
void rgbled_set_all(CRGB color);
void rgbled_clear();
```

**rgbled.cpp**

```cpp
#include "rgbled.hpp"

CRGB leds[NUM_LEDS];

void rgbled_init()
{
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);

    leds[0] = CRGB::Red;
    leds[1] = CRGB::Yellow;
    leds[2] = CRGB::Green;
    leds[3] = CRGB::Blue;
    FastLED.show();
    Serial.println("[INIT] <RGB LED> Initialized with default colors.");
}

void rgbled_set_all(CRGB color)
{
    for (int i = 0; i < NUM_LEDS; ++i)
    {
        leds[i] = color;
    }
    FastLED.show();
}

void rgbled_clear()
{
    rgbled_set_all(CRGB::Black);
}
```
RGB LED的初始化在传感器上电后进行，在使用中，可以调用`rgbled_set_all(CRGB color)`函数来设置所有LED的颜色，或者调用`rgbled_clear()`函数来清除所有LED的颜色。本项目中，不同颜色的LED灯用于表示不同的状态。更进一步的，可以分别控制每个LED的颜色，以实现更复杂的状态反馈，不过本项目中暂未实现。