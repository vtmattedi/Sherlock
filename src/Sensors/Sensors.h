#pragma once
#include <Arduino.h>

// DS18B20, DHT11 and one digital sensor/switch on each drawer.
//
// Sensors_begin() starts one sampling task; everything else is a thread-safe read of what it last
// measured. The drawers are sampled fast, debounced, and reported through a callback so the light
// can react at once; the climate sensors are slow and only ever read from a snapshot.

struct SensorReadings
{
    bool temperatureValid = false;
    float temperature = 0; ///< DS18B20, Celsius.
    bool dhtValid = false;
    float dhtTemperature = 0; ///< DHT11, Celsius.
    float dhtHumidity = 0;    ///< DHT11, percent.
    uint8_t openDrawers = 0;  ///< Bit i set: drawer i is open. Unmonitored drawers read closed.
};

/// @brief Called from the sensors task whenever the set of open drawers changes. Keep it short and
/// do not block: it delays the next sample.
typedef void (*DrawerChangeCallback)(uint8_t openMask);

/// @brief Starts the sensors and their sampling task. Register the callback first so the initial
/// drawer state is not missed.
bool Sensors_begin();
void Sensors_onDrawerChange(DrawerChangeCallback callback);

/// @brief A consistent copy of the latest readings.
SensorReadings Sensors_get();

/// @brief Whether drawer `index` has a pin assigned in board.h.
bool Sensors_drawerMonitored(uint8_t index);
/// @brief The last raw digital level of a drawer (LOW or HIGH).
uint16_t Sensors_drawerRaw(uint8_t index);
bool Sensors_ds18Connected();
