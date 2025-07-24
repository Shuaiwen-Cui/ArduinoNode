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