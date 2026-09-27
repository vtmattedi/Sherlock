#include "SimpleWS281x.h"

SimpleWS281x::SimpleWS281x(uint8_t pin, size_t ledCount)
    : _pin(pin),
      _ledCount(ledCount)
{
}

SimpleWS281x::~SimpleWS281x()
{
    if (_initialized)
        rmtDeinit(_pin);

    delete[] _pixels;
    delete[] _symbols;
}

bool SimpleWS281x::begin()
{
    if (_initialized)
        return true;

    if (_ledCount == 0)
        return false;

    _pixels = new Pixel[_ledCount];

    if (!_pixels)
        return false;

    // 24 bits/symbols per RGB LED.
    _symbols = new rmt_data_t[_ledCount * 24];

    if (!_symbols)
    {
        delete[] _pixels;
        _pixels = nullptr;
        return false;
    }

    clear();

    if (!rmtInit(
            _pin,
            RMT_TX_MODE,
            RMT_MEM_NUM_BLOCKS_1,
            RMT_FREQUENCY))
    {
        delete[] _symbols;
        delete[] _pixels;

        _symbols = nullptr;
        _pixels = nullptr;

        return false;
    }

    _initialized = true;

    // Push initial OFF state.
    return show();
}

void SimpleWS281x::setPixel(
    size_t index,
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    if (!_pixels || index >= _ledCount)
        return;

    // Keep pixels in logical RGB form. show() applies this strip's BGR wire order.
    _pixels[index].g = g;
    _pixels[index].r = r;
    _pixels[index].b = b;
}

void SimpleWS281x::setAll(
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    if (!_pixels)
        return;

    for (size_t i = 0; i < _ledCount; ++i)
    {
        setPixel(i, r, g, b);
    }
}

void SimpleWS281x::clear()
{
    if (!_pixels)
        return;

    memset(_pixels, 0, sizeof(Pixel) * _ledCount);
}

void SimpleWS281x::encodeByte(
    uint8_t value,
    rmt_data_t*& out)
{
    // WS281x sends MSB first.
    for (int bit = 7; bit >= 0; --bit)
    {
        const bool one = value & (1U << bit);

        if (one)
        {
            out->duration0 = T1H;
            out->level0 = 1;
            out->duration1 = T1L;
            out->level1 = 0;
        }
        else
        {
            out->duration0 = T0H;
            out->level0 = 1;
            out->duration1 = T0L;
            out->level1 = 0;
        }

        ++out;
    }
}

bool SimpleWS281x::show()
{
    if (!_initialized || !_pixels || !_symbols)
        return false;

    rmt_data_t* out = _symbols;

    // Encode every physical LED, including zero-valued/unused indexes. Sending a shortened frame
    // would shift the logical indexes on the strip and leave trailing LEDs at their old values.
    for (size_t i = 0; i < _ledCount; ++i)
    {
        // Measured wire order for the Sherlock strip is BGR.
        encodeByte(_pixels[i].b, out);
        encodeByte(_pixels[i].g, out);
        encodeByte(_pixels[i].r, out);
    }

    const size_t symbolCount = _ledCount * 24;

    if (!rmtWrite(
            _pin,
            _symbols,
            symbolCount,
            RMT_WAIT_FOR_EVER))
    {
        return false;
    }

    /*
     * WS281x reset/latch.
     *
     * The data line must remain LOW for >50 us.
     * Give it a conservative 80 us.
     */
    delayMicroseconds(80);

    return true;
}
