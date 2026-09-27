// #include "LightColorController.h"

// void LedHandler::SetupInfo(CRGB *leds, uint16_t size, int begin_index, int end_index)
// {
//     if (begin_index > end_index)
//     {
//         int temp = begin_index;
//         begin_index = end_index;
//         end_index = temp;
//     }
//     if (end_index >= size)
//         end_index = size - 1;

//     info.leds = leds;
//     info.size = size;
//     if (begin_index > 0)
//         info.begin = begin_index;
//     if (end_index > 0)
//         info.end = end_index;
// }

// void LedHandler::setMode(function new_fn, int color_fg, int color_bg)
// {
//     if (color_fg != INT_MAX)
//         opts.baseColor = color_fg;
//     if (color_bg != INT_MAX)
//         opts.baseColor_bg = color_bg;

//     opts.mode = new_fn;
//     if (new_fn == Solid)
//         anim = Solid_();
//     anim.Init(&info, &opts);
// }

// void LedHandler::setMode(function new_fn, CRGB color_fg, CRGB color_bg)
// {
//     setMode(new_fn,crgbToHex(color_fg),crgbToHex(color_bg));
// }

// void LedHandler::setInterval(uint16_t interval)
// {
//     _interval = interval;
// }

// void LedHandler::setDirection(direction newDir)
// {
//     opts.movement = newDir;
// }

// void LedHandler::setEffect(effect newEffect)
// {
//     opts.effects = newEffect;
// }

// void LedHandler::run()
// {

//     if (_goBackTimestamp > millis())
//     {
//         _goBackTimestamp = 0;
        
//     }
    
//     if (_finishProgBar > millis())
//     return;
    
//     if (millis() < _lastMillis + _interval || _halt)
//         return;
//     _lastMillis = millis();
//     anim.Run();
//     FastLED.show();
// }

// int LedHandler::getHex()
// {
//     return crgbToHex(opts.baseColor);
// }

// int LedHandler::crgbToHex(CRGB rgb)
// {
//     return rgb.red * 0x10000 + rgb.green * 0x100 + rgb.blue;
// }

// void LedHandler::setColor(CRGB color)
// {
//     opts.baseColor = color;
//     anim.colorChanged();
// }

// void LedHandler::setHue(uint8_t hue)
// {
//     setColor(CHSV(hue,0xff,0xff));
// }

// void LedHandler::showProgBar(uint8_t prog, uint16_t timeout_ms, CRGB fgColor, CRGB bgColor)
// {
//     _finishProgBar = millis() + timeout_ms;
//     drawProgBar(prog,fgColor,bgColor);
// }

// void LedHandler::drawProgBar(uint8_t prog, CRGB fgColor, CRGB bgColor)
// {
//     for (size_t i = info.begin; i <info.end; i++)
//     {
//         bool fg = i/info.end * 100 <= prog;
//         info.leds[i] = fg? fgColor:bgColor;
//     }
    
// }

// void LedHandler::setFinite(uint16_t timeout_ms)
// {
//     _goBackTimestamp = millis() + timeout_ms ;
// }

// void LedHandler::goToLastMode()
// {
//         ControllerOptions temp_opt = opts;
//         opts = _old_opts;
//         _old_opts = temp_opt;
//         setMode(opts.mode,opts.baseColor,opts.baseColor_bg);
// }