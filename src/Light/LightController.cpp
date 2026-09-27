#include "LightController.h"

#include <board.h>

namespace
{
constexpr uint32_t STRIP_RED = 0x0000FF;
constexpr uint32_t STRIP_GREEN = 0xFF0000;
constexpr uint32_t STRIP_BLUE = 0x00FF00;
constexpr uint32_t STRIP_YELLOW = STRIP_RED | STRIP_GREEN;
constexpr uint32_t STRIP_WHITE = 0xFFFFFF;

struct Zone
{
    size_t first;
    size_t count;
};

constexpr Zone DrawerZones[] = {
    {DRAWER_1_LED_FIRST, DRAWER_1_LED_COUNT},
    {DRAWER_2_LED_FIRST, DRAWER_2_LED_COUNT},
    {DRAWER_3_LED_FIRST, DRAWER_3_LED_COUNT},
};

uint8_t scaleChannel(uint8_t value, uint8_t brightness)
{
    return static_cast<uint8_t>((static_cast<uint16_t>(value) * brightness + 127) / 255);
}
} // namespace

LightController gLight;

LightController::LightController()
    : strip_(LED_STRIP_PIN, LED_STRIP_SIZE)
{
}

bool LightController::begin()
{
    if (started_)
        return true;

    mutex_ = xSemaphoreCreateMutexStatic(&mutexStorage_);
    if (!mutex_ || !strip_.begin())
        return false;

    started_ = true;
    return true;
}

void LightController::setPixelLocked(size_t index, uint32_t color, uint8_t brightness)
{
    // Measured packed format: 0xGGBBRR.
    const uint8_t red = color & 0xFF;
    const uint8_t blue = (color >> 8) & 0xFF;
    const uint8_t green = (color >> 16) & 0xFF;
    strip_.setPixel(index, scaleChannel(red, brightness), scaleChannel(green, brightness),
                    scaleChannel(blue, brightness));
}

void LightController::fillVisibleLocked(uint32_t color, uint8_t brightness)
{
    strip_.clear();
    for (size_t i = LED_STRIP_FIRST; i < LED_STRIP_SIZE; ++i)
        setPixelLocked(i, color, brightness);
}

void LightController::renderLocked()
{
    strip_.clear();

    if (drawersEnabled_ && state_.openDrawers != 0)
    {
        for (size_t zone = 0; zone < sizeof(DrawerZones) / sizeof(DrawerZones[0]); ++zone)
        {
            if (!(state_.openDrawers & (1u << zone)))
                continue;
            const size_t end = min(DrawerZones[zone].first + DrawerZones[zone].count,
                                   static_cast<size_t>(LED_STRIP_SIZE));
            for (size_t i = DrawerZones[zone].first; i < end; ++i)
                setPixelLocked(i, STRIP_WHITE, state_.brightness);
        }
    }
    else if (state_.on)
    {
        for (size_t i = LED_STRIP_FIRST; i < LED_STRIP_SIZE; ++i)
            setPixelLocked(i, state_.color, state_.brightness);
    }

    strip_.show();
}

void LightController::setColor(uint32_t color)
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    state_.color = color & 0xFFFFFF;
    renderLocked();
    xSemaphoreGive(mutex_);
}

void LightController::setBrightness(uint8_t brightness)
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    state_.brightness = brightness;
    renderLocked();
    xSemaphoreGive(mutex_);
}

void LightController::setOn(bool on)
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    state_.on = on;
    renderLocked();
    xSemaphoreGive(mutex_);
}

bool LightController::toggle()
{
    if (!started_)
        return false;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    state_.on = !state_.on;
    const bool on = state_.on;
    renderLocked();
    xSemaphoreGive(mutex_);
    return on;
}

void LightController::setOpenDrawers(uint8_t mask)
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    state_.openDrawers = mask & 0x07;
    renderLocked();
    xSemaphoreGive(mutex_);
}

void LightController::setDrawersEnabled(bool enabled)
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    drawersEnabled_ = enabled;
    renderLocked();
    xSemaphoreGive(mutex_);
}

LightState LightController::state() const
{
    if (!started_)
        return state_;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    const LightState copy = state_;
    xSemaphoreGive(mutex_);
    return copy;
}

void LightController::showStatusLocked(uint32_t color)
{
    fillVisibleLocked(color, 255);
    strip_.show();
}

void LightController::blinkLocked(uint32_t color, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        showStatusLocked(color);
        delay(180);
        strip_.clear();
        strip_.show();
        delay(180);
    }
    renderLocked();
}

void LightController::showStartup()
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    showStatusLocked(STRIP_YELLOW);
    xSemaphoreGive(mutex_);
}

void LightController::blinkConnected()
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    blinkLocked(STRIP_GREEN, 3);
    xSemaphoreGive(mutex_);
}

void LightController::showOtaProgress(uint8_t percent)
{
    if (!started_)
        return;
    percent = min(percent, static_cast<uint8_t>(100));
    xSemaphoreTake(mutex_, portMAX_DELAY);
    strip_.clear();
    const size_t visibleCount = LED_STRIP_SIZE - LED_STRIP_FIRST;
    const size_t completed = (visibleCount * percent + 99) / 100;
    for (size_t i = 0; i < completed; ++i)
        setPixelLocked(LED_STRIP_FIRST + i, STRIP_BLUE, 255);
    strip_.show();
    xSemaphoreGive(mutex_);
}

void LightController::showOtaFailure()
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    blinkLocked(STRIP_RED, 2);
    xSemaphoreGive(mutex_);
}

void LightController::showOtaSuccess()
{
    if (!started_)
        return;
    xSemaphoreTake(mutex_, portMAX_DELAY);
    showStatusLocked(STRIP_GREEN);
    xSemaphoreGive(mutex_);
}
