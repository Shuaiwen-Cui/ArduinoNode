# feedback

In this project, the feedback system is implemented using an RGB LED strip. Following is the relevant code:

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

The RGB LED initialization occurs after the sensor is powered on. During operation, you can call the `rgbled_set_all(CRGB color)` function to set the color of all LEDs, or call the `rgbled_clear()` function to turn off all LEDs. In this project, to cooperate with the node state machine, the `rgbled_set_by_state(NodeState state)` function is used to set the LED color based on the current node state. 