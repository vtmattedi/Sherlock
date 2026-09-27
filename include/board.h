#pragma once

// mw-overnight v1.0: ESP32-S3 with an onboard DS18B20 and DHT11, an external WS2812 strip and
// TCRT5000 reflective sensors on the drawers.

#define ONE_WIRE_BUS 16 // DS18B20, onboard
#define DHT_PIN 17      // DHT11, onboard
#define LED_STRIP_PIN 18

#define LED_STRIP_SIZE 133
// Pixels 0-33 are hidden and pixel 34 is only half visible, so rendering starts at 35.
#define LED_STRIP_FIRST 35
#define LED_STRIP_LAST 132

// Absolute physical indexes of the three drawer-handle spans.
#define DRAWER_1_LED_FIRST 48
#define DRAWER_1_LED_COUNT 9
#define DRAWER_2_LED_FIRST 80
#define DRAWER_2_LED_COUNT 9
#define DRAWER_3_LED_FIRST 112
#define DRAWER_3_LED_COUNT 9

// Two drawer switches/sensors using basic digital inputs. -1 leaves a drawer unmonitored.
#define DRAWER_1_PIN 26
#define DRAWER_2_PIN 27
#define DRAWER_3_PIN -1
#define DRAWER_COUNT 2
#define DRAWER_OPEN_LEVEL HIGH
// Consecutive agreeing samples (DRAWER_SAMPLE_MS apart) before a change is accepted.
#define DRAWER_DEBOUNCE_SAMPLES 2
#define DRAWER_SAMPLE_MS 20
