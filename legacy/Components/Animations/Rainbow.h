#pragma once
#include <Animations/AnimCore.h>

class Rainbow_ : public Animation
{
    uint8_t interval_divisor = 0;
    bool random = false;
    void InitAnim() override
    {
        uint8_t starthue = random8();
        for (size_t i = info->begin; i < info->end; i++)
        {
            info->leds[i] = CHSV(starthue + map(i, info->begin, info->end, 0, 255), 255, 255);
            ;
        }
    }
    void RunAnim() override
    {
        if (random)
        {
            interval_divisor++;
            if (interval_divisor > 40)
            {
                InitAnim();
                info->Show = true;
            }
        }

        return;
    }
};
