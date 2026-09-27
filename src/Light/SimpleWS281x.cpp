#include "SimpleWS281x.h"

SimpleWS281x::SimpleWS281x(uint8_t pin, size_t ledCount, ColorOrder colorOrder)
    : _pin(pin),
      _ledCount(ledCount),
      _colorOrder(colorOrder)
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

    // Keep pixels in logical RGB form. show() applies the configured physical wire order.
    _pixels[index].g = g;
    _pixels[index].r = r;
    _pixels[index].b = b;
}

void SimpleWS281x::setPixel(size_t index, uint32_t rgb)
{
    setPixel(index, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
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

void SimpleWS281x::setAll(uint32_t rgb)
{
    setAll((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
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
        const Pixel &pixel = _pixels[i];
        switch (_colorOrder)
        {
        case ColorOrder::RGB:
            encodeByte(pixel.r, out);
            encodeByte(pixel.g, out);
            encodeByte(pixel.b, out);
            break;
        case ColorOrder::RBG:
            encodeByte(pixel.r, out);
            encodeByte(pixel.b, out);
            encodeByte(pixel.g, out);
            break;
        case ColorOrder::GRB:
            encodeByte(pixel.g, out);
            encodeByte(pixel.r, out);
            encodeByte(pixel.b, out);
            break;
        case ColorOrder::GBR:
            encodeByte(pixel.g, out);
            encodeByte(pixel.b, out);
            encodeByte(pixel.r, out);
            break;
        case ColorOrder::BRG:
            encodeByte(pixel.b, out);
            encodeByte(pixel.r, out);
            encodeByte(pixel.g, out);
            break;
        case ColorOrder::BGR:
            encodeByte(pixel.b, out);
            encodeByte(pixel.g, out);
            encodeByte(pixel.r, out);
            break;
        }
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
