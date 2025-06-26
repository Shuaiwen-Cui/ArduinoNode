# feedback

In this project, the feedback system is implemented using an RGB LED strip. Following is the relevant code:

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

The RGB LED initialization occurs after the sensor is powered on. During operation, you can call the `rgbled_set_all(CRGB color)` function to set the color of all LEDs, or call the `rgbled_clear()` function to turn off all LEDs. In this project, different colors of the LED strip are used to indicate different statuses. For more advanced feedback, individual control of each LED's color could be implemented, but this is not included in the current project.