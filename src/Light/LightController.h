#pragma once

#include <Arduino.h>

#include "SimpleWS281x.h"

struct LightState
{
    bool on = true;
    uint32_t color = 0xFF00FF; // Yellow in the strip's measured R/B/G packed order.
    uint8_t brightness = 127;
    uint8_t openDrawers = 0;
};

class LightController
{
public:
    LightController();
    bool begin();

    void setColor(uint32_t color);
    void setBrightness(uint8_t brightness);
    void setOn(bool on);
    bool toggle();
    void setOpenDrawers(uint8_t mask);
    void setDrawersEnabled(bool enabled);

    LightState state() const;

    void showStartup();
    void blinkConnected();
    void showOtaProgress(uint8_t percent);
    void showOtaFailure();
    void showOtaSuccess();

private:
    SimpleWS281x strip_;
    mutable StaticSemaphore_t mutexStorage_;
    mutable SemaphoreHandle_t mutex_ = nullptr;
    LightState state_;
    bool drawersEnabled_ = true;
    bool started_ = false;

    void renderLocked();
    void fillVisibleLocked(uint32_t color, uint8_t brightness);
    void setPixelLocked(size_t index, uint32_t color, uint8_t brightness);
    void showStatusLocked(uint32_t color);
    void blinkLocked(uint32_t color, uint8_t count);
};

extern LightController gLight;
