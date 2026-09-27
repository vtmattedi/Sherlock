#pragma once

#include <Arduino.h>

#include "SimpleWS281x.h"

class ColourType;

constexpr uint32_t DEFAULT_COLOR = 0xFFFF00; // Yellow in standard 0xRRGGBB order.
constexpr uint8_t DEFAULT_BRIGHTNESS = 127;

struct LightState
{
    bool on = true;
    uint32_t color = DEFAULT_COLOR;
    uint8_t brightness = DEFAULT_BRIGHTNESS;
    uint8_t openDrawers = 0;
};

class LightController
{
public:
    LightController();
    bool begin();
    bool bindResources();

    void setColor(uint32_t color);
    void setColor(const ColourType &color);
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
    bool resourcesBound_ = false;

    void updateColor(uint32_t color, bool publish);
    void updateBrightness(uint8_t brightness, bool publish);
    void renderLocked();
    void fillVisibleLocked(uint32_t color, uint8_t brightness);
    void setPixelLocked(size_t index, uint32_t color, uint8_t brightness);
    void showStatusLocked(uint32_t color);
    void blinkLocked(uint32_t color, uint8_t count);
};

extern LightController gLight;
