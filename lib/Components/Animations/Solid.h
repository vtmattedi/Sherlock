#pragma once
#include <Animations/AnimCore.h>
#include <LightColorSupport.h>
extern void MqttSend(String topic, String message, bool insertOwner, bool retained);


class Solid_ : public Animation
{
    void InitAnim() override
    {
        // MqttSend("Sherlock/debug", "Init - child", false, false);
        for (size_t i = info->begin; i < info->end; i++)
        {
            info->leds[i] = CRGB(options->baseColor);
        }
        // String s = "";
        // char buffer[300];
        // sprintf(buffer, "Solid INIT: color = 0x%6x DA= 0x%6x", options->getHexD(options->baseColor), options->getHexD(info->leds[50]));
        // s += buffer;
        // s += "";

        // MqttSend(
        //     "Sherlock/debug",
        //     s,
        //     false,
        //     false);
    }
    void RunAnim() override
    {
        return;
    }
};
