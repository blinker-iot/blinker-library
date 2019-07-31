#ifndef BLINKER_MIOT_H
#define BLINKER_MIOT_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BLINKERMIOT
{
    public :
        BLINKERMIOT() {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            Blinker.attachMIOTSetPowerState(newFunction);
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachMIOTSetPowerState(newFunction);
        }

        void attachColor(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachMIOTSetColor(newFunction);
        }

        void attachMode(blinker_callback_with_uint8_arg_t newFunction)
        {
            Blinker.attachMIOTSetMode(newFunction);
        }

        // void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        // {
        //     Blinker.attachMIOTSetcMode(newFunction);
        // }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachMIOTSetBrightness(newFunction);
        }

        // void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        // {
        //     Blinker.attachMIOTRelativeBrightness(newFunction);
        // }

        void attachColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachMIOTSetColorTemperature(newFunction);
        }

        // void attachRelativeColorTemperature(blinker_callback_with_int32_arg_t newFunction)
        // {
        //     Blinker.attachMIOTRelativeColorTemperature(newFunction);
        // }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            Blinker.attachMIOTQuery(newFunction);
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachMIOTQuery(newFunction);
        }

        void powerState(const String & state, uint8_t num)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");

            if (state == "on")
            {
                payload += "True";
            }
            else
            {
                payload += "False";
            }
            
            if (num != 0)
            {
                payload += BLINKER_F("\",\"num\":");
                payload += STRING_format(num);
            }
            else
            {
                payload += BLINKER_F("\"");
            }            

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void powerState(const String & state)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");

            if (state == "on")
            {
                payload += "True";
            }
            else
            {
                payload += "False";
            }
            
            payload += BLINKER_F("\"");

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(uint32_t clr)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR_);
            payload += BLINKER_F("\":");
            payload += STRING_format(clr);
            payload += BLINKER_F(",");
            payload += BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR);
            payload += BLINKER_F("\":");
            payload += STRING_format(clr);

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(uint8_t md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":");
            payload += STRING_format(md);

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLORTEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(clrTemp);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(bright);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

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
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // Blinker.aligeniePrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void co2(double _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void co2(float _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void co2(int _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String miData;

            if (_fresh >> 0 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aCtemp;
                
                free(aCtemp);
            }

            if (_fresh >> 4 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 5 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 6 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 7 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aPm25;
                
                free(aPm25);
            }

            if (_fresh >> 8 & 0x01) {
                if (miData.length()) miData += BLINKER_F(",");
                else miData += BLINKER_F("{");
                
                miData += aCO2;
                
                free(aCO2);
            }

            miData += BLINKER_F("}");

            _fresh = 0;

            Blinker.miotPrint(miData);
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
        char * aCO2;  
        uint16_t _fresh = 0;
};

#endif
