#pragma once

// mw-overnight v1.0: ESP32-S3 with an onboard DS18B20 and DHT11, an external WS2812 strip and
// magnetic (reed) contacts on the three doors/windows, one per handle LED span.

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

// Three magnetic on/off contacts (doors/windows) on basic digital inputs. -1 leaves one unmonitored.
// Each contact switches the pin to GND when its magnet is near (closed), so the internal pull-up
// makes an open door/window read HIGH. Wire them the other way round and change both defines.
// DRAWERS_ENABLED 0 removes the feature entirely: no pins are configured, nothing is sampled, and
// no drawer resources or config are published. The pins below are kept for re-enabling.
#define DRAWERS_ENABLED 0
#if DRAWERS_ENABLED
#define DRAWER_1_PIN 26
#define DRAWER_2_PIN 27
#define DRAWER_3_PIN 23
#else
#define DRAWER_1_PIN -1
#define DRAWER_2_PIN -1
#define DRAWER_3_PIN -1
#endif
#define DRAWER_COUNT 3
#define DRAWER_INPUT_MODE INPUT_PULLUP
#define DRAWER_OPEN_LEVEL HIGH
// Consecutive agreeing samples (DRAWER_SAMPLE_MS apart) before a change is accepted.
#define DRAWER_DEBOUNCE_SAMPLES 2
#define DRAWER_SAMPLE_MS 20
