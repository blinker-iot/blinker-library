#ifndef BLINKER_ALIGENIE_H
#define BLINKER_ALIGENIE_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BLINKERALIGENIE
{
    public :
        BLINKERALIGENIE() {}

        void attachPowerState(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetPowerState(newFunction);
        }

        void attachColor(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetColor(newFunction);
        }

        void attachBrightness(callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetBrightness(newFunction);
        }

        void attachRelativeBrightness(callback_with_int32_arg_t newFunction)
        {
            Blinker.attachRelativeBrightness(newFunction);
        }

        void attachColorTemperature(callback_with_int32_arg_t newFunction)
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

        void powerState(const String & state)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_POWERSTATE) + 
                "\":\"" + state + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void color(const String & clr)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_COLOR) + 
                "\":\"" + clr + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void colorTemp(int clrTemp)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_COLORTEMP) + 
                "\":\"" + STRING_format(clrTemp) + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void brightness(int bright)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_BRIGHTNESS) + 
                "\":\"" + STRING_format(bright) + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void temp(int _temp)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_TEMP) + 
                "\":\"" + STRING_format(_temp) + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void humi(int _humi)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_HUMI) + 
                "\":\"" + STRING_format(_humi) + "\"}";

            Blinker.aligeniePrint(payload);
        }

        void pm25(int _pm25)
        {
            String payload = "{\"" + STRING_format(BLINKER_CMD_PM25) + 
                "\":\"" + STRING_format(_pm25) + "\"}";

            Blinker.aligeniePrint(payload);
        }
};

#endif