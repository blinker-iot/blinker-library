#ifndef BLINKER_DUEROS_H
#define BLINKER_DUEROS_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BLINKERDUEROS
{
    public :
        BLINKERDUEROS() {}

        void attachPowerState(blinker_callback_with_string_uint8_arg_t newFunction)
        {
            Blinker.attachDuerOSSetPowerState(newFunction);
        }

        void attachPowerState(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachDuerOSSetPowerState(newFunction);
        }

        void attachColor(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachDuerOSSetColor(newFunction);
        }

        void attachMode(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachDuerOSSetMode(newFunction);
        }

        void attachCancelMode(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachDuerOSSetcMode(newFunction);
        }

        void attachBrightness(blinker_callback_with_string_arg_t newFunction)
        {
            Blinker.attachDuerOSSetBrightness(newFunction);
        }

        void attachRelativeBrightness(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachDuerOSRelativeBrightness(newFunction);
        }

        void attachQuery(blinker_callback_with_int32_uint8_arg_t newFunction)
        {
            Blinker.attachDuerOSQuery(newFunction);
        }

        void attachQuery(blinker_callback_with_int32_arg_t newFunction)
        {
            Blinker.attachDuerOSQuery(newFunction);
        }

        void powerState(const String & state, uint8_t num)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_POWERSTATE);
            payload += BLINKER_F("\":\"");
            payload += state;
            if (num != 0)
            {
                payload += BLINKER_F("\",\"num\":");
                payload += STRING_format(num);
            }
            else
            {
                payload += BLINKER_F("\"");
            }

            // Blinker.DuerOSPrint(payload);

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
            payload += state;
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 0 & 0x01) {
                free(aState);
            }

            aState = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aState, payload.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(int32_t clr)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR_);
            payload += BLINKER_F("\":\"");
            payload += clr;
            payload += BLINKER_F("\",");
            payload += BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_COLOR);
            payload += BLINKER_F("\":\"");
            payload += clr;
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 1 & 0x01) {
                free(aColor);
            }

            aColor = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aColor, payload.c_str());

            _fresh |= 0x01 << 1;
        }

        void mode(const String & now_md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":[\"\",\"");
            payload += now_md;
            payload += BLINKER_F("\"]");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 2 & 0x01) {
                free(aMode);
            }

            aMode = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aMode, payload.c_str());

            _fresh |= 0x01 << 2;
        }

        void mode(const String & pre_md, const String & now_md)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_MODE);
            payload += BLINKER_F("\":[\"");
            payload += pre_md;
            payload += BLINKER_F("\",\"");
            payload += now_md;
            payload += BLINKER_F("\"]");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 2 & 0x01) {
                free(aMode);
            }

            aMode = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aMode, payload.c_str());

            _fresh |= 0x01 << 2;
        }

        void brightness(int now_bright)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":[\"\",");
            payload += STRING_format(now_bright);
            payload += BLINKER_F("]");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 3 & 0x01) {
                free(aBright);
            }

            aBright = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aBright, payload.c_str());

            _fresh |= 0x01 << 3;
        }

        void brightness(int pre_bright, int now_bright)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_BRIGHTNESS);
            payload += BLINKER_F("\":[");
            payload += STRING_format(pre_bright);
            payload += BLINKER_F(",");
            payload += STRING_format(now_bright);
            payload += BLINKER_F("]");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 3 & 0x01) {
                free(aBright);
            }

            aBright = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aBright, payload.c_str());

            _fresh |= 0x01 << 3;
        }

        void temp(double _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void temp(float _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void temp(int _temp)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TEMP);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_temp);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 4 & 0x01) {
                free(aTemp);
            }

            aTemp = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTemp, payload.c_str());

            _fresh |= 0x01 << 4;
        }

        void humi(double _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void humi(float _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void humi(int _humi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_HUMI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_humi);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 5 & 0x01) {
                free(aHumi);
            }

            aHumi = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aHumi, payload.c_str());

            _fresh |= 0x01 << 5;
        }

        void pm25(double _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void pm25(float _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void pm25(int _pm25)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM25);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm25);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 6 & 0x01) {
                free(aPm25);
            }

            aPm25 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm25, payload.c_str());

            _fresh |= 0x01 << 6;
        }

        void pm10(double _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm10(float _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void pm10(int _pm10)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_PM10);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_pm10);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 7 & 0x01) {
                free(aPm10);
            }

            aPm10 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aPm10, payload.c_str());

            _fresh |= 0x01 << 7;
        }

        void co2(double _co2)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_CO2);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_co2);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

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

            // Blinker.DuerOSPrint(payload);

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

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 8 & 0x01) {
                free(aCO2);
            }

            aCO2 = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aCO2, payload.c_str());

            _fresh |= 0x01 << 8;
        }

        void aqi(int _aqi)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_AQI);
            payload += BLINKER_F("\":\"");
            payload += STRING_format(_aqi);
            payload += BLINKER_F("\"");

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 9 & 0x01) {
                free(aAQI);
            }

            aAQI = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aAQI, payload.c_str());

            _fresh |= 0x01 << 9;
        }        

        void time(uint32_t _time)
        {
            String payload = BLINKER_F("\"");
            payload += STRING_format(BLINKER_CMD_TIME_ALL);
            payload += BLINKER_F("\":");
            payload += STRING_format(_time/1000);

            // Blinker.DuerOSPrint(payload);

            if (_fresh >> 10 & 0x01) {
                free(aTIME);
            }

            aTIME = (char*)malloc((payload.length()+1)*sizeof(char));
            strcpy(aTIME, payload.c_str());

            _fresh |= 0x01 << 10;
        }

        void print()
        {
            if (_fresh == 0) return;
            
            String duerData;

            if (_fresh >> 0 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aState;
                
                free(aState);
            }

            if (_fresh >> 1 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aColor;
                
                free(aColor);
            }

            if (_fresh >> 2 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aMode;
                
                free(aMode);
            }

            if (_fresh >> 3 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aBright;
                
                free(aBright);
            }

            if (_fresh >> 4 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aTemp;
                
                free(aTemp);
            }

            if (_fresh >> 5 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aHumi;
                
                free(aHumi);
            }

            if (_fresh >> 6 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aPm25;
                
                free(aPm25);
            }

            if (_fresh >> 7 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aPm10;
                
                free(aPm10);
            }

            if (_fresh >> 8 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aCO2;
                
                free(aCO2);
            }

            if (_fresh >> 9 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aAQI;
                
                free(aAQI);
            }

            if (_fresh >> 10 & 0x01) {
                if (duerData.length()) duerData += BLINKER_F(",");
                else duerData += BLINKER_F("{");
                
                duerData += aTIME;
                
                free(aTIME);
            }

            duerData += BLINKER_F("}");

            _fresh = 0;

            Blinker.duerPrint(duerData);
        }

    private :
        char * aState;
        char * aColor;
        char * aMode;
        char * aBright;
        char * aTemp;
        char * aHumi;
        char * aPm25;
        char * aPm10;
        char * aCO2;        
        char * aAQI;
        char * aTIME;
        uint16_t _fresh = 0;
};

#endif
