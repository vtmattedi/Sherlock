

#ifndef LIGHTCOLORSUPPORT
#define LIGHTCOLORSUPPORT
#include <Arduino.h>
#include <FastLED.h>

enum function
{
    Solid,
    ShowHandles,
    lightShow,
    randomLights,
    rainbow,
    laser,
    ShowHandlesRandom,
};

function getFnbyName(String fnName)
{
    if (fnName == "Solid")
        return Solid;
    if (fnName == "ShowHandles")
        return ShowHandles;
    if (fnName == "lightShow")
        return lightShow;
    if (fnName == "randomLights")
        return randomLights;
    if (fnName == "rainbow")
        return rainbow;
    if (fnName == "laser")
        return laser;
    return Solid;
}

String getfnName(int fnValue)
{
    switch (fnValue)
    {
    case Solid:
        return "Solid";
    case ShowHandles:
        return "ShowHandles";
    case lightShow:
        return "lightShow";
    case randomLights:
        return "randomLights";
    case rainbow:
        return "rainbow";
    case laser:
        return "laser";
    default:
        return "Unknown";
    }
}

String getFns()
{
    String enumString = "";
    bool done = false;
    for (int i = 0; !done; i++)
    {
        String current_fn = getfnName(i);
        if (current_fn == "Unknown")
            done = true;
        else
        {
            enumString += current_fn;
            enumString += ",";
            enumString += i;
            enumString += "+";
        }
    }
    return enumString;
}

enum direction
{
    backward = -1,
    stop = 0,
    foward = 1,
};

direction opposite(direction currentDirection)
{
    switch (currentDirection)
    {
    case backward:
        return foward;
    case foward:
        return backward;
    default:
        return stop;
    }
}

enum effect
{
    none,
    blink,
    fade,
};

struct ControllerOptions
{
    uint16_t interval = 25;
    CRGB baseColor = CRGB::Red;
    CRGB baseColor_bg = CRGB::Blue;
    function mode = function::Solid;
    direction movement = direction::stop;
    bool loop_movement = true;
    effect effects = effect::none;

    ControllerOptions(
        function Mode = function::Solid,
        CRGB BaseColor = CRGB::Red,
        CRGB BaseColor_bg = CRGB::Blue,
        uint16_t Interval = 25,
        direction Movement = direction::stop,
        effect Effects = effect::none)
    {
        interval = Interval;
        baseColor = BaseColor;
        baseColor_bg = BaseColor_bg;
        mode = Mode;
        movement = Movement;
        effects = Effects;
    }
    int getHexD(CRGB b)
    {
        return b.red * 0x10000 + b.g * 0x100 + b.b;
    }
    String toString()
    {
        String s;
        s += "{ Mode: ";
        s += mode;
        s += " BaseColor: ";
        s += getHexD(baseColor);
        s += " BaseColor_bg: ";
        s += getHexD(baseColor_bg);
        s += " Movement: ";
        s += movement;
        s += " Effects: ";
        s += effects;
        s += " interval: ";
        s += interval;
        s += " loop_movement: ";
        s += loop_movement;
        s += " }";
        return s;
    }
};

struct LedInfo
{
    CRGB *leds;
    uint16_t size;
    uint16_t begin;
    uint16_t end;
    bool Show = false;
    String toString()
    {
        char buffer[30];
        sprintf(buffer, "%p", leds);
        String s;
        s = "{ leds: 0x";
        Serial.println();
        s += String(buffer);
        s += " size: ";
        s += size;
        s += " begin: ";
        s += begin;
        s += " end: ";
        s += end;
        s += " }";
        return s;
    }
};

#endif