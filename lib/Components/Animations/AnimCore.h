#ifndef ANIMCORE
#define ANIMCORE
#include <FastLED.h>
#include <Arduino.h>
#include <LightColorSupport.h>

extern void MqttSend(String topic, String message, bool insertOwner, bool retained);


// useless for our usecase
#define SIZE(array) sizeof(array) / sizeof(array[0])

class Animation
{
public:
    /// @brief Tick specific Animation
    virtual void RunAnim(){};
    /// @brief Start specific Animation
    virtual void InitAnim(){MqttSend("Sherlock/debug", "Init - [WRONG]papi", false, false);};

public:
    /// @brief User options, such as preferred color, movement, and effects
    ControllerOptions *options;
    /// @brief Info about the led array, pointer to array, size etc
    LedInfo *info;
    uint8_t step;
    /// @brief flag to ignore movement inside leed
    bool ignore_movement = false;
    bool ignore_effects = false;

    void Init(LedInfo *led_info, ControllerOptions *opt)
    {
        FastLED.clear();
        ///MqttSend("Sherlock/debug","Init - parent",false,false);
        options = opt;
        info = led_info;
        InitAnim();
    };
    void applyEffects()
    {
        step++;
        if (options->effects == blink)
        {
            for (size_t i = 0; i < info->size; i++)
            {
                info->leds[i] = info->leds[i] * step % 2;
            }
        }
        else if (options->effects == fade)
        {
            FastLED.setBrightness(step);
        }
    }

    void partialMove(direction dir, int startIndex, int endIndex, bool loop = true)
    {
        if (dir == stop)
            return;

        // Serial.printf("Start P.M, d = %d, s = %d, e = %d,\n", dir, startIndex, endIndex);
        // Serial.println("");
        if (dir == foward)
        {
            CRGB first = info->leds[startIndex];

            for (size_t i = startIndex; i < endIndex; i++)
            {
                // Serial.printf("[%d] - > [%d]\n", i, i + 1);
                info->leds[i] = info->leds[i + 1];
            }
            if (loop)
                info->leds[endIndex] = first;
        }
        else if (dir == backward)
        {
            CRGB last = info->leds[endIndex];

            for (size_t i = 0; i < endIndex - startIndex; i++)
            {
                // Serial.printf("[%d] - > [%d]\n", endIndex - i, endIndex - (i + 1));
                info->leds[endIndex - i] = info->leds[endIndex - (i + 1)];
            }
            if (loop)
                info->leds[startIndex] = last;
        }

        return;
    }

    void Run()
    {
        info->Show = false;
        RunAnim();
        if (!ignore_movement)
            partialMove(options->movement, 0, info->size, options->loop_movement);
        if (!ignore_effects)
            applyEffects();
    }

    void colorChanged()
    {
        InitAnim();
    }
};

#endif