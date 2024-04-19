#pragma once
#include <Animations/AnimCore.h>

#define HANDLE_SIZE 10
#define HANDLE_1_INDEX 19
#define HANDLE_2_INDEX 50
#define HANDLE_3_INDEX 83

class DrawerHandles_ : public Animation
{
    uint8_t interval_divisor = 0;
    bool random = false;
    void InitAnim() override
    {
        for (size_t i = 0; i < HANDLE_SIZE; i++)
        {
            info->leds[HANDLE_1_INDEX + i] = options->baseColor;

            info->leds[HANDLE_2_INDEX + i] = options->baseColor;

            info->leds[HANDLE_3_INDEX + i] = options->baseColor;
        }
    }
    void RunAnim() override
    {
        if (random)
        {
            interval_divisor++;
            if (interval_divisor > 40)
            {
                interval_divisor = 0;
                uint8_t newhue = random8();
                for (size_t i = 0; i < HANDLE_SIZE; i++)
                {
                    info->leds[HANDLE_1_INDEX + i] = CHSV(newhue, 255, 255);
                    ;

                    info->leds[HANDLE_2_INDEX + i] = CHSV(newhue + 85, 255, 255);

                    info->leds[HANDLE_3_INDEX + i] = CHSV(newhue + 170, 255, 255);
                }
                info->Show = true;
            }
        }

        return;
    }
};
