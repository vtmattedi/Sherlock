#pragma once

#ifndef LIGHTCOLORCONTROLLER
#define LIGHTCOLORCONTROLLER

#include <FastLED.h>
#include <Arduino.h>
#include <LightColorSupport.h>
#include <Animations/Animations.h>

#define LED_STRIP_PIN 18
#define LED_STRIP_SIZE 102
#define NUM_LEDS LED_STRIP_SIZE

extern void MqttSend(String topic, String message, bool insertOwner, bool retained);
/******HOW TO USE******
1 - Create an CRGB array
2 - Start FastLED {at Setup}
    I.E. - FastLED.addLeds<WS2812, LED_STRIP_PIN, GRB>(leds, LED_STRIP_SIZE)
3 - call LedHandler::SetupInfo {at Setup}
    3.5 - Repeat 2 and 3 for each separete controller.
4 - call LedHandler::Run() {at Loop}
**********************/
class LedHandler
{

private:
    LedInfo info;
    Animation *anim = new Animation();
    unsigned long _lastMillis = 0;
    int _interval = 25;
    bool _halt = false;
    uint32_t _finishProgBar = 0;
    uint32_t _goBackTimestamp = 0;
    ControllerOptions _old_opts;

    void drawProgBar(uint8_t, CRGB, CRGB);

public:
    ControllerOptions opts;
    function current_fn = function::Solid;

    void SetupInfo(CRGB *leds, uint16_t size, int begin_index = -1, int end_index = -1);
    void setMode(function new_fn, int color_fg = INT_MAX, int color_bg = INT_MAX);
    void setMode(function new_fn, CRGB color_fg, CRGB color_bg = CRGB::Black);
    void setInterval(uint16_t interval);
    void setDirection(direction newDir);
    void setEffect(effect newEffect);
    int getHex();
    void run();
    int crgbToHex(CRGB rgb);
    void setColor(CRGB color);
    void setHue(uint8_t hue);
    void showProgBar(uint8_t prog, uint16_t timeout, CRGB fgColor, CRGB bgColor);
    void setFinite(uint16_t timeout);
    void goToLastMode();
    uint32_t lastreport = 0;
    void report()
    {
        // String s = String("opts: ") + opts.toString() + String("\ninfo: ") + info.toString();

        // s += "\n leds: \n";

        // for (size_t i = 0; i < 10; i++)
        // {
        //     char buffer[30];
        //     sprintf(buffer, "led[%d] = 0x%6x  | ", i * 10, crgbToHex(leds[i * 10]));
        //     s += buffer;

        // }

        // MqttSend(
        //     "Sherlock/debug",
        //     s,
        //     false,
        //     false);
        lastreport = millis();
    }
};

#include "LightColorController.h"

void LedHandler::SetupInfo(CRGB *leds, uint16_t size, int begin_index, int end_index)
{
    if (begin_index > end_index)
    {
        int temp = begin_index;
        begin_index = end_index;
        end_index = temp;
    }
    if (end_index >= size)
        end_index = size - 1;

    info.leds = leds;
    info.size = size;
    if (begin_index > 0)
        info.begin = begin_index;
    if (end_index > 0)
        info.end = end_index;
}

void LedHandler::setMode(function new_fn, int color_fg, int color_bg)
{
    _old_opts = opts;
    opts.mode = new_fn;
    // opts = ControllerOptions();
    // opts.baseColor = _old_opts.baseColor;
    // opts.baseColor_bg = _old_opts.baseColor_bg;
    if (anim)
        delete anim;

    if (color_fg != INT_MAX)
        opts.baseColor = color_fg;
    if (color_bg != INT_MAX)
        opts.baseColor_bg = color_bg;

    opts.mode = new_fn;
    if (new_fn == Solid)
        anim = new Solid_();
    else if (new_fn == ShowHandles)
        anim = new DrawerHandles_();
    else if (new_fn == ShowHandles)
        anim = new DrawerHandles_();
    else if (new_fn == rainbow)
        anim = new Rainbow_();
    else
    {
        MqttSend("Sherlock/Debug", String("Mode: [") + new_fn + String("] Not Yet implemented! defaulting to Solid."));
    }
    if (!anim)
        anim = new Solid_();
    anim->Init(&info, &opts);
    FastLED.show();
}

void LedHandler::setMode(function new_fn, CRGB color_fg, CRGB color_bg)
{
    setMode(new_fn, crgbToHex(color_fg), crgbToHex(color_bg));
}

void LedHandler::setInterval(uint16_t interval)
{
    _interval = interval;
}

void LedHandler::setDirection(direction newDir)
{
    opts.movement = newDir;
}

void LedHandler::setEffect(effect newEffect)
{
    opts.effects = newEffect;
}

void LedHandler::run()
{
    if (millis() - lastreport > 5000)
    {
        report();
    }

    if (_goBackTimestamp > millis())
    {
        _goBackTimestamp = 0;
        goToLastMode();
    }

    if (_finishProgBar > millis())
        return;

    if (millis() < _lastMillis + _interval || _halt)
        return;
    _lastMillis = millis();
    anim->Run();
    if (info.Show)
        FastLED.show();
}

int LedHandler::getHex()
{
    return crgbToHex(opts.baseColor);
}

int LedHandler::crgbToHex(CRGB rgb)
{
    return rgb.red * 0x10000 + rgb.green * 0x100 + rgb.blue;
}

void LedHandler::setColor(CRGB color)
{
    opts.baseColor = color;
    MqttSend("Sherlock/debug", "Color Changed!", false, false);
    anim->colorChanged();
    FastLED.show();
}

void LedHandler::setHue(uint8_t hue)
{
    setColor(CHSV(hue, 0xff, 0xff));
}

void LedHandler::showProgBar(uint8_t prog, uint16_t timeout_ms, CRGB fgColor, CRGB bgColor)
{
    _finishProgBar = millis() + timeout_ms;
    drawProgBar(prog, fgColor, bgColor);
}

void LedHandler::drawProgBar(uint8_t prog, CRGB fgColor, CRGB bgColor)
{
    for (size_t i = info.begin; i < info.end; i++)
    {
        bool fg = i * 100 <= prog * (info.end - info.begin);
        info.leds[i] = fg ? fgColor : bgColor;
    }
    String debug = "prog = ";
    debug += prog;
    debug += " | fg val: ";
    debug += prog * (info.end - info.begin);
    debug += " | fg: ";
    debug += 50 * 100 <= prog * (info.end - info.begin);

    MqttSend("Sherlock/debug", debug, false, false);
    FastLED.show();
}

void LedHandler::setFinite(uint16_t timeout_ms)
{
    _goBackTimestamp = millis() + timeout_ms;
}

void LedHandler::goToLastMode()
{
    ControllerOptions temp_opt = opts;
    opts = _old_opts;
    _old_opts = temp_opt;
    setMode(opts.mode, opts.baseColor, opts.baseColor_bg);
}

LedHandler ledController;

#endif