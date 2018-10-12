#ifndef BLINKER_ALIGENIE_H
#define BLINKER_ALIGENIE_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BLINKERALIGENIE
{
    public :
        BLINKERALIGENIE() {}

        void attachSetPowerState(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetPowerState(newFunction);
        }

        void attachSetColor(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetColor(newFunction);
        }

        void attachSetBrightness(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetBrightness(newFunction);
        }

        void attachRelativeBrightness(callback_with_int32_arg_t newFunction)
        {
            Blinker.attachRelativeBrightness(newFunction);
        }

        void attachSetColorTemperature(callback_with_int32_arg_t newFunction)
        {
            Blinker.attachSetColorTemperature(newFunction);
        }

        void attachRelativeColorTemperature(callback_with_int32_arg_t newFunction)
        {
            Blinker.attachRelativeColorTemperature(newFunction);
        }

        void attachQuery(callback_with_int32_arg_t newFunction)
        {
            Blinker.attachQuery(newFunction);
        }
};

#endif