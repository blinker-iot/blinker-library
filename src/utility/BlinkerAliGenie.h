#ifndef BLINKER_ALIGENIE_H
#define BLINKER_ALIGENIE_H

#include "Blinker/BlinkerConfig.h"
#include "utility/BlinkerUtility.h"

class BLINKERALIGENIE
{
    public :
        BLINKERALIGENIE() {}

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetPowerState(newFunction);
        }

        void attachColor(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetColor(newFunction);
        }

        void attachMode(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetMode(newFunction);
        }

        void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetcMode(newFunction);
        }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachSetBrightness(newFunction);
        }

        void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachRelativeBrightness(newFunction);
        }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachSetColorTemperature(newFunction);
        }

        void attachRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachRelativeColorTemperature(newFunction);
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachQuery(newFunction);
        }

        void powerState(const String & state)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_POWERSTATE) + 
                "\":\"" + state + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & clr)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_COLOR) + 
                "\":\"" + clr + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(const String & md)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_MODE) + 
                "\":\"" + md + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 2 & 0x01) {
                free(aMode);
            }

            aMode = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aMode, payload.c_str());

            _fresh |= 0x01 << 2;
        }

        void colorTemp(int clrTemp)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_COLORTEMP) + 
                "\":\"" + STRING_format(clrTemp) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 3 & 0x01) {
                free(aCtemp);
            }

            aCtemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCtemp, payload.c_str());

            _fresh |= 0x01 << 3;
        }

        void brightness(int bright)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_BRIGHTNESS) + 
                "\":\"" + STRING_format(bright) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aBright);
            }

            aBright = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aBright, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void temp(double _temp)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_TEMP) + 
                "\":\"" + STRING_format(_temp) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void temp(float _temp)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_TEMP) + 
                "\":\"" + STRING_format(_temp) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void temp(int _temp)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_TEMP) + 
                "\":\"" + STRING_format(_temp) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void humi(double _humi)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_HUMI) + 
                "\":\"" + STRING_format(_humi) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void humi(float _humi)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_HUMI) + 
                "\":\"" + STRING_format(_humi) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void humi(int _humi)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_HUMI) + 
                "\":\"" + STRING_format(_humi) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void pm25(double _pm25)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_PM25) + 
                "\":\"" + STRING_format(_pm25) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm25(float _pm25)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_PM25) + 
                "\":\"" + STRING_format(_pm25) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm25(int _pm25)
        {
            String payload = "\"" + STRING_format(BLINKER_CMD_PM25) + 
                "\":\"" + STRING_format(_pm25) + "\"";

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String aliData;

            if (_fresh >> 0 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aCtemp;
                
                free(aCtemp);
            }

            if (_fresh >> 4 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 5 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 6 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 7 & 0x01) {
                if (aliData.length()) aliData += BLINKER_F(",");
                else aliData += BLINKER_F("{");
                
                aliData += aPm25;
                
                free(aPm25);
            }

            aliData += BLINKER_F("}");

            _fresh = 0;

            Blinker.aligeniePrint(aliData);
        }

    private :
        char * aState;
        char * aColor;
        char * aMode;
        char * aCtemp;
        char * aBright;
        char * aTemp;
        char * aHumi;
        char * aPm25;
        uint8_t _fresh = 0;
};

#endif