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
