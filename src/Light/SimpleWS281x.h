#pragma once

#include <Arduino.h>
#include <esp32-hal-rmt.h>

enum Color : uint32_t
{
    COLOR_BLACK   = 0x000000,
    COLOR_WHITE   = 0xFFFFFF,

    COLOR_RED     = 0xFF0000,
    COLOR_GREEN   = 0x00FF00,
    COLOR_BLUE    = 0x0000FF,

    COLOR_YELLOW  = 0xFFFF00,
    COLOR_CYAN    = 0x00FFFF,
    COLOR_MAGENTA = 0xFF00FF,

    COLOR_ORANGE  = 0xFF8000,
    COLOR_PURPLE  = 0x8000FF,
    COLOR_PINK    = 0xFF4080,

    COLOR_LIME    = 0x80FF00,
    COLOR_TEAL    = 0x008080,

    COLOR_GRAY    = 0x808080,
    COLOR_DARK_GRAY = 0x404040,
    COLOR_LIGHT_GRAY = 0xC0C0C0,
};
class SimpleWS281x
{
public:
    SimpleWS281x(uint8_t pin, size_t ledCount);
    ~SimpleWS281x();

    bool begin();

    void setPixel(size_t index, uint8_t r, uint8_t g, uint8_t b);
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    void clear();

    bool show();

    size_t size() const
    {
        return _ledCount;
    }

private:
    struct Pixel
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    uint8_t _pin;
    size_t _ledCount;

    Pixel* _pixels = nullptr;
    rmt_data_t* _symbols = nullptr;

    bool _initialized = false;

    static constexpr uint32_t RMT_FREQUENCY = 10000000; // 10 MHz = 100 ns/tick

    // WS2812/WS2818-class timing at 100 ns resolution.
    //
    // 0:
    //   HIGH ~0.4 us
    //   LOW  ~0.85 us
    //
    // 1:
    //   HIGH ~0.8 us
    //   LOW  ~0.45 us
    //
    static constexpr uint16_t T0H = 4;
    static constexpr uint16_t T0L = 9;

    static constexpr uint16_t T1H = 8;
    static constexpr uint16_t T1L = 5;

    void encodeByte(uint8_t value, rmt_data_t*& out);
};
