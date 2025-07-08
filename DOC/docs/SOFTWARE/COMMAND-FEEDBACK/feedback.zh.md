# 反馈

本项目中的节点状态反馈是通过外接的RGB LED灯来实现的。以下是相关代码：

**rgbled.hpp**

```cpp
#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "nodestate.hpp"

#define NUM_LEDS 4
#define LED_PIN 7

extern CRGB leds[NUM_LEDS];

void rgbled_init();
void rgbled_set_all(CRGB color);
void rgbled_clear();
void rgbled_set_by_state(NodeState state);
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

void rgbled_set_by_state(NodeState state)
{
    CRGB color;

    switch (state)
    {
    case NodeState::BOOT:                color = CRGB(0xFFFFFF); break; // White
    case NodeState::IDLE:                color = CRGB(0x008000); break; // Green
    case NodeState::PREPARING:           color = CRGB(0xFFFF00); break; // Yellow
    case NodeState::SAMPLING:            color = CRGB(0x800080); break; // Purple
    case NodeState::RF_COMMUNICATING:    color = CRGB(0x00FFFF); break; // Cyan
    case NodeState::WIFI_COMMUNICATING:  color = CRGB(0x0000FF); break; // Blue
    case NodeState::ERROR:               color = CRGB(0xFF0000); break; // Red
    default:                             color = CRGB::Black;    break;
    }

    rgbled_set_all(color);
}

```

RGB LED的初始化在传感器上电后进行，在使用中，可以调用`rgbled_set_all(CRGB color)`函数来设置所有LED的颜色，或者调用`rgbled_clear()`函数来清除所有LED的颜色。本项目中，为了配合状态机的使用并提供更有效的反馈，我们还实现了`rgbled_set_by_state(NodeState state)`函数，根据节点的状态来设置LED的颜色，每次切换状态时都可以调用此函数来更新LED的颜色。